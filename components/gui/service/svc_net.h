#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 以太网状态（供以太网页展示） */
typedef struct {
    bool link_up;          /* 物理链路是否 up */
    bool has_ip;           /* 是否已获取到 IP */
    char ip[16];           /* IP 地址（点分十进制） */
    char gw[16];           /* 网关 */
    char netmask[16];      /* 子网掩码 */
} net_status_t;

/* 查询以太网当前状态。UI 层只依赖本函数，不直接碰 esp_netif。 */
void svc_net_get_status(net_status_t *out);

#ifdef __cplusplus
}
#endif
