#include "wifi_server.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// Forward declaration for poll callback (lwIP expects err_t return type)
static err_t poll_close_cb(void *arg, struct tcp_pcb *tpcb);

static uint16_t global_r = 0, global_g = 0, global_b = 0;
static float global_correctness = 0.0f;
static bool global_success_locked = false; // Lock at 97%

// --- HTML CODE (Stored as a string) ---
// This includes a script that automatically fetches /data every 500ms
static const char *html_page_body =
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
    "let hasTriggeredSuccess = false;"
    "function update(){"
    "  fetch('/data?t=' + Date.now(), {cache: 'no-store'})"
    "    .then(r => r.json())"
    "    .then(data => {"
    "      document.getElementById('r').innerText = data.r;"
    "      document.getElementById('g').innerText = data.g;"
    "      document.getElementById('b').innerText = data.b;"
    "      document.getElementById('per').innerText = Math.round(data.correctness);"
    "      document.getElementById('bar').style.width = data.correctness + '%';"
    "      document.getElementById('colorBox').style.backgroundColor = 'rgb(' + (data.r/10) + ',' + (data.g/10) + ',' + (data.b/10) + ')';"
    "      if (data.success === true && !hasTriggeredSuccess) {"
    "        hasTriggeredSuccess = true;"
    "        setTimeout(() => { window.location = '/success'; }, 500);"
    "      }"
    "    })"
    "    .catch(err => console.log('Error:', err));"
    "}"
    "setInterval(update, 500);"
    "update();"
    "</script></body></html>";

// Success page for >= 97% accuracy
static const char *success_page_body =
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "body { font-family: sans-serif; text-align: center; background: linear-gradient(135deg, #2a5 0%, #1a3 100%); color: white; min-height: 100vh; display: flex; flex-direction: column; justify-content: center; align-items: center; margin: 0; padding: 20px; }"
    "h1 { font-size: 48px; margin: 20px 0; animation: pulse 1s infinite; }"
    ".sequence { font-size: 32px; font-weight: bold; margin: 30px 0; letter-spacing: 10px; }"
    ".pattern { font-size: 18px; margin: 20px 0; }"
    ".back-link { margin-top: 40px; }"
    ".back-link a { color: white; text-decoration: none; padding: 10px 20px; border: 2px solid white; border-radius: 5px; }"
    "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.7; } }"
    "</style></head>"
    "<body>"
    "<h1>🎉 SUCCESS! 🎉</h1>"
    "<div class=\"sequence\">2, 6, 18, 54, X</div>"
    "<div class=\"pattern\">Geometric Sequence<br>Each term × 3</div>"
    "<div class=\"back-link\"><a href=\"/\">Back to Calibration</a></div>"
    "</body></html>";

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;
    tcp_recved(tpcb, p->tot_len);

    // 0. Check if SUCCESS page requested
    if (strstr(request, "GET /success"))
    {
        int body_len = strlen(success_page_body);
        char header[256];
        int header_len = snprintf(header, sizeof(header),
                                  "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html; charset=utf-8\r\n"
                                  "Connection: close\r\n"
                                  "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                                  "Pragma: no-cache\r\n"
                                  "Content-Length: %d\r\n\r\n",
                                  body_len);
        tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, success_page_body, body_len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_close(tpcb);
    }
    // 1. Check if the browser is asking for DATA (JSON)
    else if (strstr(request, "GET /data"))
    {
        char json_body[128];
        snprintf(json_body, sizeof(json_body), "{\"r\":%d,\"g\":%d,\"b\":%d,\"correctness\":%.1f,\"success\":%s}",
                 global_r, global_g, global_b, global_correctness,
                 global_success_locked ? "true" : "false");

        char header[256];
        int body_len = strlen(json_body);
        int header_len = snprintf(header, sizeof(header),
                                  "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Connection: close\r\n"
                                  "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                                  "Pragma: no-cache\r\n"
                                  "Content-Length: %d\r\n\r\n",
                                  body_len);

        tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, json_body, body_len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_close(tpcb);
    }
    // 2. Otherwise, send the DASHBOARD (HTML)
    else
    {
        int body_len = strlen(html_page_body);
        char header[256];
        int header_len = snprintf(header, sizeof(header),
                                  "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html; charset=utf-8\r\n"
                                  "Connection: close\r\n"
                                  "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                                  "Pragma: no-cache\r\n"
                                  "Content-Length: %d\r\n\r\n",
                                  body_len);

        tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, html_page_body, body_len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_close(tpcb);
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_callback);
    tcp_err(newpcb, NULL); // optional: we could set a real err callback
    // Close lingering connections via poll callback
    tcp_poll(newpcb, poll_close_cb, (u8_t)4);
    tcp_nagle_disable(newpcb);
    tcp_setprio(newpcb, TCP_PRIO_NORMAL);
    return ERR_OK;
}

// Poll callback: close connection to avoid leaks if lingering
static err_t poll_close_cb(void *arg, struct tcp_pcb *tpcb)
{
    (void)arg;
    tcp_close(tpcb);
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
    // Once success is locked, freeze the display values
    if (global_success_locked)
    {
        // Don't update anymore - keep showing success state
        return;
    }

    // Update display with current values
    global_r = r;
    global_g = g;
    global_b = b;
    global_correctness = correctness;

    // Lock success state at 97%
    if (correctness >= 97.0f)
    {
        global_success_locked = true;
        global_correctness = 97.0f; // Freeze at 97% so browser sees consistent state
    }
}

void wifi_poll(void)
{
    cyw43_arch_poll();
}

bool wifi_is_success_locked(void)
{
    return global_success_locked;
}