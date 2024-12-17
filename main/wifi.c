#include "./wifi_config.c"
#include "esp_bit_defs.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_interface.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "freertos/event_groups.h"
#include "portmacro.h"
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/_intsup.h>

static EventGroupHandle_t s_wifi_event_group;

static int attempts = 3;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    printf("station " MACSTR " join, AID=%d\n", MAC2STR(event->mac),
           event->aid);

  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    printf("station " MACSTR " leave, AID=%d\n", MAC2STR(event->mac),
           event->aid);
  }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {

    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (attempts-- > 0) {
      esp_wifi_connect();
      printf("retrying connection...\n");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      printf("connection attempt failed\n");
      connection_success = false;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    printf("got ip: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    connection_success = true;
  }
}


esp_netif_t *wifi_ap_start() {
  esp_netif_init();
  s_wifi_event_group = xEventGroupCreate();
  esp_netif_t *netif = esp_netif_create_default_wifi_ap();
  wifi_init_config_t wf_cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wf_cfg);
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL, NULL);
  wifi_config_t ap_cfg = {
      .ap =
          {
              .ssid = "ESP32-config-AP",
              .ssid_len = strlen("ESP32-config-AP"),
              .channel = 1,
              .password = "1234SBC2024",
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA_WPA2_PSK,
              .pmf_cfg = {.required = true},
          },
  };

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_cfg);
  esp_wifi_start();

  printf("\nAP started with ssid=%s, password=%s\n", ap_cfg.ap.ssid, ap_cfg.ap.password);
  return netif;
}

esp_netif_t *wifi_sta_start(char *ssid, char *pass) {
	attempts = 5;
  esp_netif_init();
  s_wifi_event_group = xEventGroupCreate();
  esp_netif_t *netif = esp_netif_create_default_wifi_sta();
  wifi_init_config_t wf_cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wf_cfg);

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &event_handler, NULL, &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &event_handler, NULL, &instance_got_ip);

  wifi_config_t sta_cfg = {
      .sta =
          {
              .ssid = "",
              .password = "",
              .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
              .pmf_cfg.required = false,
          },
  };

  strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
  strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
  esp_wifi_start();
  printf("starting connection with credentials %s, %s\n", ssid, pass);

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & WIFI_CONNECTED_BIT) {
    printf("connected to STA.\n");
  } else {
    printf("connection failed\n");
  }

  return netif;
}

void wifi_stop(esp_netif_t *netif) {
  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_netif_destroy_default_wifi(netif);
}