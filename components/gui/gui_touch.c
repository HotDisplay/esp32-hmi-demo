#include "gui_touch.h"

#include <string.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TOUCH_QUEUE_LENGTH 64
#define TOUCH_READ_BATCH   16

typedef struct {
    lv_point_t point;
    lv_indev_state_t state;
    uint32_t timestamp;
    bool reset;
} touch_sample_t;

static struct {
    esp_lcd_touch_handle_t touch;
    lv_indev_t *indev;
    TaskHandle_t task;
    QueueHandle_t queue;
    touch_sample_t current;
    uint32_t tick_offset;
    unsigned batch_count;
} s_touch;

static const char *TAG = "TOUCH_INPUT";

static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (xQueueReceive(s_touch.queue, &s_touch.current, 0) == pdTRUE) {
        if (s_touch.current.reset) {
            lv_indev_reset(indev, NULL);
            s_touch.current.reset = false;
        }
    }
    data->point = s_touch.current.point;
    data->state = s_touch.current.state;
    data->timestamp = s_touch.current.timestamp;
    bool pending = uxQueueMessagesWaiting(s_touch.queue) != 0;
    data->continue_reading = pending && ++s_touch.batch_count < TOUCH_READ_BATCH;
    if (!data->continue_reading) {
        s_touch.batch_count = 0;
        if (pending) {
            lvgl_port_task_wake(LVGL_PORT_EVENT_TOUCH, indev);
        }
    }
}

static void IRAM_ATTR interrupt_cb(esp_lcd_touch_handle_t touch)
{
    (void)touch;
    BaseType_t woken = pdFALSE;
    if (s_touch.task != NULL) {
        vTaskNotifyGiveFromISR(s_touch.task, &woken);
        if (woken) {
            portYIELD_FROM_ISR();
        }
    }
}

static void sample_task(void *arg)
{
    (void)arg;
    touch_sample_t sample = { 0 };
    unsigned samples = 0;
    uint32_t first = 0, last = 0, max_gap = 0;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_lcd_touch_point_data_t point;
        uint8_t count = 0;
        esp_err_t err = esp_lcd_touch_read_data(s_touch.touch);
        if (err == ESP_OK) {
            err = esp_lcd_touch_get_data(s_touch.touch, &point, &count, 1);
        }
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "touch read failed: %s", esp_err_to_name(err));
            sample.reset = true;
            continue;
        }

        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        if (count != 0) {
            if (sample.state != LV_INDEV_STATE_PRESSED) {
                samples = 0;
                first = now;
                max_gap = 0;
            } else if (now - last > max_gap) {
                max_gap = now - last;
            }
            samples++;
            last = now;
            sample.point.x = point.x;
            sample.point.y = point.y;
        } else if (sample.state == LV_INDEV_STATE_PRESSED && samples > 1) {
            ESP_LOGI(TAG, "stroke: %u samples, mean gap %lu ms, max gap %lu ms",
                     samples, (unsigned long)((last - first) / (samples - 1)), (unsigned long)max_gap);
        }
        sample.state = count ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        sample.timestamp = now - s_touch.tick_offset;
        if (xQueueSend(s_touch.queue, &sample, 0) != pdTRUE) {
            /* 丢样时重新起笔，避免跨过缺失点误连长线。 */
            ESP_LOGW(TAG, "touch queue full, restarting stroke");
            xQueueReset(s_touch.queue);
            sample.reset = true;
            xQueueSend(s_touch.queue, &sample, 0);
        }
        sample.reset = false;
        lvgl_port_task_wake(LVGL_PORT_EVENT_TOUCH, s_touch.indev);
    }
}

lv_indev_t *gui_touch_add(lv_display_t *display, esp_lcd_touch_handle_t touch)
{
    if (display == NULL || touch == NULL || s_touch.indev != NULL) {
        return NULL;
    }
    if (touch->config.int_gpio_num == GPIO_NUM_NC) {
        const lvgl_port_touch_cfg_t cfg = { .disp = display, .handle = touch };
        return lvgl_port_add_touch(&cfg);
    }

    s_touch.queue = xQueueCreate(TOUCH_QUEUE_LENGTH, sizeof(touch_sample_t));
    if (s_touch.queue == NULL) {
        return NULL;
    }
    if (!lvgl_port_lock(0)) {
        vQueueDelete(s_touch.queue);
        s_touch.queue = NULL;
        return NULL;
    }
    s_touch.touch = touch;
    s_touch.tick_offset = (uint32_t)(esp_timer_get_time() / 1000) - lv_tick_get();
    s_touch.indev = lv_indev_create();
    if (s_touch.indev != NULL) {
        lv_indev_set_type(s_touch.indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(s_touch.indev, display);
        lv_indev_set_read_cb(s_touch.indev, read_cb);
        /* TIMER 模式允许一次消费多个点；读定时器暂停，由采集任务唤醒。 */
        lv_timer_pause(lv_indev_get_read_timer(s_touch.indev));
        if (xTaskCreate(sample_task, "touch_read", 4096, NULL, 11, &s_touch.task) == pdPASS) {
            esp_err_t err = esp_lcd_touch_register_interrupt_callback(touch, interrupt_cb);
            if (err == ESP_OK) {
                lvgl_port_unlock();
                return s_touch.indev;
            }
            ESP_LOGW(TAG, "touch interrupt registration failed: %s", esp_err_to_name(err));
            esp_lcd_touch_register_interrupt_callback(touch, NULL);
            vTaskDelete(s_touch.task);
        }
        lv_indev_delete(s_touch.indev);
    }
    vQueueDelete(s_touch.queue);
    memset(&s_touch, 0, sizeof(s_touch));
    lvgl_port_unlock();
    return NULL;
}
