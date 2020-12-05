/*
Pinos-WeMos             Função                  Pino-ESP-8266
        TX                      TXD                             TXD/GPIO1
        RX                      RXD                             RXD/GPIO3
        D0                      IO                              GPIO16  
        D1                      IO, SCL                 GPIO5
        D2                      IO, SDA                 GPIO4
        D3                      IO, 10k PU              GPIO0
        D4                      IO, 10k PU, LED 		GPIO2
        D5                      IO, SCK                 GPIO14
        D6                      IO, MISO                GPIO12
        D7                      IO, MOSI                GPIO13
        D0                      IO, 10k PD, SS  		GPIO15
        A0                      Inp. AN 3,3Vmax A0
*/
#include "stdio.h"
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "dht.h"
#include "ultrasonic.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <esp_http_server.h>

#define TRUE                1
#define FALSE               0
#define DEBUG               TRUE

#define LED_BUILDING        GPIO_NUM_2
#define BUTTON              GPIO_NUM_0

#define WIFI_SSID           CONFIG_WIFI_SSID
#define WIFI_PASS           CONFIG_WIFI_PASSWORD

#define WIFI_CONNECTING_BIT BIT0
#define WIFI_CONNECTED_BIT  BIT1
#define WIFI_FAIL_BIT       BIT2

#define MAX_DISTANCE_CM     500
#define TRIGGER_GPIO        4
#define ECHO_GPIO           5

#define PORT                CONFIG_SERVER_PORT

typedef struct
{
  int16_t temperatura;
  int16_t umidade;
  int32_t distancia;
} DataSensor;

static const char * TAG = "wifi station: ";
static EventGroupHandle_t s_wifi_event_group;

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = 16;
static httpd_handle_t server = NULL;

DataSensor data;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_init_sta(void);

void task_sinalizarConexaoWifi(void *pvParameter);
void task_reconectarWifi(void *pvParameter);
void task_LerTemperaturaUmidade(void *pvParameter);
void task_lerDistancia(void *pvParameter);
void app_main(void);

esp_err_t temperatura_get_handler(httpd_req_t *req)
{
    char temperatura[50];

    sprintf(temperatura, "%d", data.temperatura);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "10000");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");

    httpd_resp_send(req, temperatura, strlen(temperatura));

    return ESP_OK;
}

httpd_uri_t temperatura = {
    .uri       = "/temperatura",
    .method    = HTTP_GET,
    .handler   = temperatura_get_handler
};

esp_err_t umidade_get_handler(httpd_req_t *req)
{
    char umidade[50];

    sprintf(umidade, "%d", data.umidade);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "10000");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");

    httpd_resp_send(req, umidade, strlen(umidade));

    return ESP_OK;
}

httpd_uri_t umidade = {
    .uri       = "/umidade",
    .method    = HTTP_GET,
    .handler   = umidade_get_handler
};

esp_err_t distancia_get_handler(httpd_req_t *req)
{
    char distancia[50];

    sprintf(distancia, "%d", data.distancia);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "10000");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");

    httpd_resp_send(req, distancia, strlen(distancia));

    return ESP_OK;
}

httpd_uri_t distancia = {
    .uri       = "/distancia",
    .method    = HTTP_GET,
    .handler   = distancia_get_handler
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {

        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &temperatura);
        httpd_register_uri_handler(server, &umidade);
        httpd_register_uri_handler(server, &distancia);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Conectando...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);

        esp_wifi_connect();

        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT | WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Conexão falhou...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT | WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Conectado...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT | WIFI_FAIL_BIT);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado!! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));

        server = start_webserver();
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void task_sinalizarConexaoWifi(void *pvParameter)
{
    bool estadoLED = 0;

    gpio_set_direction(LED_BUILDING, GPIO_MODE_OUTPUT);

    while(TRUE)
    {
        EventBits_t eventBitsWifi = xEventGroupGetBits(s_wifi_event_group);

        if(eventBitsWifi & WIFI_CONNECTING_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(500 / portTICK_RATE_MS);
        }
        else if(eventBitsWifi & WIFI_FAIL_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(100 / portTICK_RATE_MS);
        }
        else if(eventBitsWifi & WIFI_CONNECTED_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}

void task_reconectarWifi(void *pvParameter)
{
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);


    while(TRUE)
    {   
        xEventGroupWaitBits(s_wifi_event_group, WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        if(!gpio_get_level(BUTTON))
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);

            esp_wifi_connect();
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void task_LerTemperaturaUmidade(void *pvParameters)
{
	int16_t temperatura;
	int16_t umidade;

    while (1)
    {
    	if(dht_read_data(sensor_type, dht_gpio, &umidade, &temperatura) == ESP_OK)
    	{
    		data.temperatura = temperatura / 10;
	    	data.umidade = umidade / 10;
    	}

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task_lerDistancia(void *pvParamters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);

    uint32_t distancia;

    while (1)
    {
        esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distancia);

        if (res != ESP_OK)
        {
            printf("Error: ");

            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%d\n", res);
            }
        }
        else
        {
        	data.distancia = distancia;
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta();

    xTaskCreate(task_sinalizarConexaoWifi, "task_sinalizarConexaoWifi", 2048, NULL, 5, NULL );
    xTaskCreate(task_reconectarWifi, "task_reconectarWifi", 2048, NULL, 5, NULL );

    xTaskCreate(task_LerTemperaturaUmidade, "task_LerTemperaturaUmidade", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(task_lerDistancia, "task_lerDistancia", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}