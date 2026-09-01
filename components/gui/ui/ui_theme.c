#include "ui_theme.h"

/* 全局主题初始化。
 *
 * 骨架阶段仅占位：颜色 / 字体统一从 ui_theme.h 的宏取值，页面文件不写死。
 * UI 设计师接手后，可在此集中注册共享样式（卡片、标题、按钮、状态条等），
 * 例如导出一个 lv_style_t *ui_theme_get_style(ui_style_id_t id) 供各页面复用。 */
void ui_theme_init(void)
{
    /* TODO: 集中定义全局默认样式 */
}
