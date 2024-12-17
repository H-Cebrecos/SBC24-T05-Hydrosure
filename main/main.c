#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "AS726X.h"
#include "./http.c"
#include "./wifi.c"
#include "./screen.c"
#include "./ota.h"
#include <math.h>

#define MARGIN 50
#define MARGIN_R 100
#define MARGIN_S 200
#define MARGIN_T 25
#define MARGIN_U 25
#define MARGIN_V 100
#define MARGIN_W 20

#define THINGSBOARD_HOST "http://demo.thingsboard.io"
#define THINGSBOARD_TOKEN "wp7zjjhwtefbde0cboml"

static const char *TAG = "THINGSBOARD_AS726X";

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS 1000

#define GPIO_INPUT_IO_15 15
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_INPUT_IO_15)

esp_err_t post_data(float calR, float calS, float calT, float calU, float calV, float calW, uint8_t temperature, uint8_t pot)
{
    char json_data[256];
    snprintf(json_data, sizeof(json_data),
             "{\"calR\": %.2f, \"calS\": %.2f, \"calT\": %.2f, \"calU\": %.2f, \"calV\": %.2f, \"calW\": %.2f, \"temperature\": %d, \"nopot\": %d }",
             calR, calS, calT, calU, calV, calW, temperature, pot);

    char url[256];
    snprintf(url, sizeof(url), "%s/api/v1/%s/telemetry", THINGSBOARD_HOST, THINGSBOARD_TOKEN);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Datos enviados: %s", json_data);
    }
    else
    {
        ESP_LOGE(TAG, "Error al enviar datos: %s", esp_err_to_name(err));
    }
    return err;
}

const char *text = "      Potable";
const char *text2 = "  No Potable";
const char *Text_no_sample = "   No Sample ";
const char *text_init = "   HYDROSURE ";
const char *text_server = "   HTTP SERVER";
const char *text_server2 = "   ssid=ignore";
const char *text_server3 = "     to skip";
const char *text_cal = " calibration";
const char *text_button = " press to sample";
const char *text_fail = " wifi failed";
const char *text_salt = " Salt content";
const char *text_done = "     Done";
const char *text_skip = " Press to skip";

float cal_r, cal_s, cal_t, cal_u, cal_v, cal_w;
void app_main(void)
{
    bool skip = false;
    SSD1306_t *dev;
    esp_netif_t *netif = NULL;
    nvs_flash_init();

    esp_event_loop_create_default();

    i2c_port_t i2c_num = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_num, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_num, conf.mode,
                                       I2C_MASTER_RX_BUF_DISABLE,
                                       I2C_MASTER_TX_BUF_DISABLE, 0));

    dev = init_screen();
    screen_write(dev, text_init);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
        screen_write(dev, text_skip);
    for (int j= 0; j <= 1000; j++){
       if(gpio_get_level(GPIO_INPUT_IO_15) == 1){
        skip = true;
        break;
       }
       vTaskDelay(10 / portTICK_PERIOD_MS);

    }
    screen_write(dev, text_done);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    if (!skip){
    connection_success = false;
    netif = wifi_sta_start(ssid, passw);
    while (!connection_success)
    {
        screen_write(dev, text_fail);
        vTaskDelay(pdMS_TO_TICKS(1000));
        wifi_stop(netif);

        netif = wifi_ap_start();
        new_config = false;
        start_config_server();

        screen_write(dev, text_server2);
        screen_write_0(dev, text_server);
        screen_write_2(dev, text_server3);
        while (!new_config)
            vTaskDelay(pdMS_TO_TICKS(1000));

        wifi_stop(netif);
        const char *ignore = "ignore";
        if (strcmp(ssid, ignore) == 0)
            break;
        netif = wifi_sta_start(ssid, passw);
    }

    }

    vTaskDelay(pdMS_TO_TICKS(500));
    AS726X_t sensor;
    if (AS726X_init(&sensor, i2c_num, AS726X_ADDR, 3, 3))
    {
        ESP_LOGI(TAG, "Sensor AS726X inicializado correctamente");
    }
    else
    {
        ESP_LOGE(TAG, "Error al inicializar el sensor AS726X");
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    uint8_t version = AS726X_getVersion(&sensor);
    ESP_LOGI(TAG, "Versión del sensor: 0x%02X", version);



    screen_write(dev, text_cal);

    float calR = 0.0;
    float calS = 0.0;
    float calT = 0.0;
    float calU = 0.0;
    float calV = 0.0;
    float calW = 0.0;
    uint8_t pot = 1;



    for (int i = 0; i <= 5; i++)
    {
        while (gpio_get_level(GPIO_INPUT_IO_15) != 1)
        {
            vTaskDelay(30 / portTICK_PERIOD_MS);
        };

        AS726X_takeMeasurementsWithBulb(&sensor);
        calR += AS726X_getCalibratedR(&sensor);
        calS += AS726X_getCalibratedS(&sensor);
        calT += AS726X_getCalibratedT(&sensor);
        calU += AS726X_getCalibratedU(&sensor);
        calV += AS726X_getCalibratedV(&sensor);
        calW += AS726X_getCalibratedW(&sensor);
    }
    calR /= 6;
    calS /= 6;
    calT /= 6;
    calU /= 6;
    calV /= 6;
    calW /= 6;
    screen_write(dev, text_done);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    screen_write(dev, text_button);

    int count = 0;
    while (1)
    {   if(netif){wifi_stop(netif);}
        while (gpio_get_level(GPIO_INPUT_IO_15) != 1)
        {
            vTaskDelay(30 / portTICK_PERIOD_MS);
        };
        netif = wifi_sta_start(ssid, passw);
        AS726X_takeMeasurementsWithBulb(&sensor);
        cal_r = AS726X_getCalibratedR(&sensor);
        cal_s = AS726X_getCalibratedS(&sensor);
        cal_t = AS726X_getCalibratedT(&sensor);
        cal_u = AS726X_getCalibratedU(&sensor);
        cal_v = AS726X_getCalibratedV(&sensor);
        cal_w = AS726X_getCalibratedW(&sensor);
        uint8_t tempC = AS726X_getTemperature(&sensor);

        ESP_LOGI(TAG, "Valores calibrados:\ncalR: %.2f\ncalS: %.2f\ncalT: %.2f\ncalU: %.2f\ncalV: %.2f\ncalW: %.2f\nTemperatura: %d°C\n",
                 calR, calS, calT, calU, calV, calW, tempC);
        

        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (cal_r <= 500)
        {
            screen_write(dev, Text_no_sample);
            count++;
            if (count == 3){
                 ota_check();
            }
        }
        else
        {
            pot = 0;
            count = 0;
            if (
                fabs(calS - cal_s) <= MARGIN_S &&
                fabs(calT - cal_t) <= MARGIN_T &&
                fabs(calU - cal_u) <= MARGIN_U &&
                fabs(calV - cal_v) <= MARGIN_V &&
                fabs(calW - cal_w) <= MARGIN_W)
            {
                screen_write(dev, text);
            }
            else
            {
                pot = 1;
                if ( //(calR - 3220.0) >= MARGIN_R &&
                    fabs(calT - cal_t) <= MARGIN_T &&
                    fabs(calU - cal_u) <= MARGIN_U &&
                    (calW - cal_w) >= MARGIN_W &&
                    (calV - cal_v) >= MARGIN_V)
                {
                    screen_write(dev, text_salt);
                }
                else
                {
                    screen_write(dev, text2);
                }
            }
        }
        if (!skip){
        post_data(cal_r, cal_s, cal_t, cal_u, cal_v, cal_w, tempC, pot);
        }
        // vTaskDelay(3000 / portTICK_PERIOD_MS);
        // screen_write(dev, text2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
