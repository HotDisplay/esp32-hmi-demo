#include "svc_net.h"

#include <string.h>

#include "esp_netif.h"

/* 默认以太网 netif 的 if_key（ESP_NETIF_DEFAULT_ETH 的 if_key） */
#define ETH_NETIF_IFKEY  "ETH_DEF"

void svc_net_get_status(net_status_t *out)
{
    memset(out, 0, sizeof(*out));

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey(ETH_NETIF_IFKEY);
    if (netif == NULL) {
        /* 以太网未初始化（如该目标未开 EMAC），保持全 0 即可 */
        return;
    }

    out->link_up = esp_netif_is_netif_up(netif);

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        out->has_ip = (ip_info.ip.addr != 0);
        esp_ip4addr_ntoa(&ip_info.ip,      out->ip,      sizeof(out->ip));
        esp_ip4addr_ntoa(&ip_info.gw,      out->gw,      sizeof(out->gw));
        esp_ip4addr_ntoa(&ip_info.netmask, out->netmask, sizeof(out->netmask));
    }
}
