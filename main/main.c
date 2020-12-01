#include <stdio.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include <string.h>
#include "driver/gpio.h"

#include "wifi.h"
#include "http_client.h"
#include "json_utils.h"

#define KTOC(k) (k-273)

#define IPSTACKKEY CONFIG_IP_STACK_KEY
#define WEATHEMAPKEY CONFIG_OPEN_WEATHER_MAP_KEY

#define BUFFER_SIZE 1024
#define LED 2

#define DEBUG true

char * response_buffer = NULL;
int response_size = 0;

int wifi_connected = 0;

xSemaphoreHandle conexaoWifiSemaphore;
xSemaphoreHandle ledSemaphore;

void ledBlink(){
  gpio_set_level(LED, 0);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  gpio_set_level(LED, 1);
}

void RealizaHTTPRequest(void * params)
{
  int sucess=0;
  while(true)
  {
    if(xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      if (response_size == 0){
        response_buffer = (char *)malloc(BUFFER_SIZE);
        if (response_buffer == NULL){
          ESP_LOGE("malloc", "Memory alloc error");
          vTaskDelay(500 / portTICK_PERIOD_MS);
          continue;
        }
      }else{
        free(response_buffer); response_buffer=NULL;
        response_size = 0;
        continue;
      }
      ESP_LOGI("Main Task", "Realiza HTTP Request");
      // char url[1024] = "http://quotes.rest/qod";
      char url[1024] = "http://api.ipstack.com/check?access_key=";
      strcat(url, IPSTACKKEY);
      strcat(url, "&output=json");
      http_request(url);
      ledBlink();
      if (response_size != 0){
        response_buffer[response_size] = '\0';
        if(DEBUG) printf("%s\n\n\n", response_buffer);
        double lat=0, lng=0;
        getJsonLatLng(response_buffer, &lat, &lng);
        if(DEBUG) printf("%lf %lf\n\n", lat, lng);
        free(response_buffer); response_buffer=NULL;
        response_size = 0;

        response_buffer = (char *)malloc(BUFFER_SIZE);
        if (response_buffer == NULL){
          ESP_LOGE("malloc", "Memory alloc error");
          vTaskDelay(500 / portTICK_PERIOD_MS);
          continue;
        }

        sprintf(url, "http://api.openweathermap.org/data/2.5/weather?lat=%lf&lon=%lf&appid=%s", lat, lng, WEATHEMAPKEY);
        http_request(url);
        ledBlink();

        if (response_size != 0){
          sucess = 1;
          response_buffer[response_size] = '\0';
          double temp[3];
          int hum=-1;
          getJsonTempHum(response_buffer, temp, &hum);
          printf("Temperatura atual: %.2lf oC\n", KTOC(temp[0]));
          printf("Temperatura minima: %.2lf oC\n", KTOC(temp[1]));
          printf("Temperatura maxima: %.2lf oC\n", KTOC(temp[2]));
          printf("Humidade atual: %d %% \n", hum);
        }
        free(response_buffer); response_buffer=NULL;
        response_size = 0;
      }
      //https_request();
      xSemaphoreGive(conexaoWifiSemaphore);
    }
    if(sucess){
      if(DEBUG) vTaskDelay(1000 * 20 / portTICK_PERIOD_MS);
      else vTaskDelay(1000 * 60 * 5 / portTICK_PERIOD_MS);
      sucess=0;
    }
  }
}

void ledHandler(void * params){
  while(true){
    if(xSemaphoreTake(ledSemaphore, portMAX_DELAY)){
      if(wifi_connected){
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED, 1);
      }else{
        ledBlink();
      }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    xSemaphoreGive(ledSemaphore);
  }
}

void app_main(void)
{
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa LED
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    ledSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(ledSemaphore);
    xTaskCreate(&ledHandler,  "Controla LED", 4096, NULL, 1, NULL);
    
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    wifi_start();

    xTaskCreate(&RealizaHTTPRequest,  "Processa HTTP", 4096, NULL, 1, NULL);
 
}
