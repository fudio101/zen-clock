#pragma once

#include "bsp.h"
#include "sntp_sync.h"
#include "wifi_manager.h"
#include "ble_provisioning.h"

void on_button_press(int btn_id, bsp_btn_event_t event);
void on_sntp_sync(sntp_sync_event_t event);
void on_ble_prov_event(ble_prov_event_t event, const char *ssid, const char *pass);
void on_wifi_event(wifi_manager_event_t event);

/** Register action callbacks for nav system (Reset WiFi, etc.) */
void app_handlers_register_nav_callbacks(void);
