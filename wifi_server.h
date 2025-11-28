#ifndef wifi_server
#define wifi_server

#include <stdint.h>
#include <stdbool.h>

// Initialize Wi-Fi in Access Point Mode
void wifi_init_ap(const char *ssid, const char *password);

// Update the data that gets sent to the web browser
void wifi_update_data(uint16_t r, uint16_t g, uint16_t b, float correctness);

// Poll function to handle network traffic (call in main loop)
void wifi_poll(void);

#endif