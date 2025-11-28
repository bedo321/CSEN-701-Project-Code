#include "wifi_server.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include <string.h>
#include <stdio.h>

static uint16_t global_r = 0, global_g = 0, global_b = 0;
static float global_correctness = 0.0f;

// --- HTML CODE (Stored as a string) ---
// This includes a script that automatically fetches /data every 500ms
static const char *html_page =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "body { font-family: sans-serif; text-align: center; background: #1a1a1a; color: white; }"
    ".box { width: 100px; height: 100px; margin: 20px auto; border: 2px solid #fff; }"
    ".bar-container { width: 80%; background: #444; margin: 10px auto; height: 30px; border-radius: 15px; }"
    ".bar { height: 100%; background: #4caf50; width: 0%; border-radius: 15px; transition: width 0.5s; }"
    "</style></head>"
    "<body>"
    "<h1>Spectral Interface</h1>"
    "<h3>Current Mix</h3>"
    "<div id=\"colorBox\" class=\"box\"></div>"
    "<h3>Correctness: <span id=\"per\">0</span>%</h3>"
    "<div class=\"bar-container\"><div id=\"bar\" class=\"bar\"></div></div>"
    "<p>R: <span id=\"r\">0</span> G: <span id=\"g\">0</span> B: <span id=\"b\">0</span></p>"
    "<script>"
    "setInterval(function() {"
    "  fetch('/data').then(response => response.json()).then(data => {"
    "    document.getElementById('r').innerText = data.r;"
    "    document.getElementById('g').innerText = data.g;"
    "    document.getElementById('b').innerText = data.b;"
    "    document.getElementById('per').innerText = data.correctness;"
    "    document.getElementById('bar').style.width = data.correctness + '%';"
    "    document.getElementById('colorBox').style.backgroundColor = 'rgb(' + data.r/10 + ',' + data.g/10 + ',' + data.b/10 + ')';"
    "  });"
    "}, 1500);" // Refresh every 500ms
    "</script></body></html>";

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    // 1. Check if the browser is asking for DATA (JSON)
    if (strstr(request, "GET /data"))
    {
        char json_body[128];
        snprintf(json_body, sizeof(json_body), "{\"r\":%d,\"g\":%d,\"b\":%d,\"correctness\":%.1f}",
                 global_r, global_g, global_b, global_correctness);

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
                 strlen(json_body), json_body);

        tcp_write(tpcb, header, strlen(header), TCP_WRITE_FLAG_COPY);
    }
    // 2. Otherwise, send the DASHBOARD (HTML)
    else
    {
        tcp_write(tpcb, html_page, strlen(html_page), TCP_WRITE_FLAG_COPY);
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

void wifi_init_ap(const char *ssid, const char *password)
{
    if (cyw43_arch_init())
        return;
    cyw43_arch_enable_ap_mode(ssid, password, CYW43_AUTH_WPA2_AES_PSK);

    struct netif *n = &cyw43_state.netif[CYW43_ITF_AP];
    ip4_addr_t ip, mask;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    netif_set_ipaddr(n, &ip);
    netif_set_netmask(n, &mask);
    netif_set_up(n);

    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
}

void wifi_update_data(uint16_t r, uint16_t g, uint16_t b, float correctness)
{
    global_r = r;
    global_g = g;
    global_b = b;
    global_correctness = correctness;
}

void wifi_poll(void)
{
    cyw43_arch_poll();
}