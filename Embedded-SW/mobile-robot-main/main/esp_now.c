
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"

#include "include/esp_now_structures.h"

#define ESPNOW_MAXDELAY 5120
#define ESP_NOW_RATE 100

static const char *ESP_NOW_TAG = "ESP_NOW";

// Define the mobile robot's ESP32 MAC Address here!
static uint8_t s_mobile_robot_address[ESP_NOW_ETH_ALEN] = {0x70, 0x04, 0x1D, 0xCD, 0xBC, 0x50};

// Define the USB Dongle ESP32 MAC Address here!
static uint8_t s_usb_dongle_address[ESP_NOW_ETH_ALEN] = {0xD4, 0xF9, 0x8D, 0x04, 0x1A, 0xCC};

// Global Variables
mobile_robot_state_info_t state_data;
mobile_robot_command_t robot_cmd;

esp_now_peer_info_t dongle_info;

// WiFi should start before using ESPNOW 
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK( esp_wifi_start());
}

static void data_send_cb(const uint8_t *mac_address, esp_now_send_status_t status)
{
    ESP_LOGI(ESP_NOW_TAG,"Data send status: %i",status);
}
