#include <stdio.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include <string.h>

#include "wifi.h"
#include "http_client.h"
#include "json_utils.h"

#define IPSTACKKEY CONFIG_IP_STACK_KEY
#define WEATHEMAPKEY CONFIG_OPEN_WEATHER_MAP_KEY

#define BUFFER_SIZE 1024

char * response_buffer = NULL;
int response_size = 0;

xSemaphoreHandle conexaoWifiSemaphore;

void RealizaHTTPRequest(void * params)
{
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
      if (response_size != 0){
        response_buffer[response_size] = '\0';
        printf("%s\n\n\n", response_buffer);
        double lat=0, lng=0;
        getJsonLatLng(response_buffer, &lat, &lng);
        printf("%lf %lf\n\n", lat, lng);
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

        if (response_size != 0){
          response_buffer[response_size] = '\0';
          double temp[3];
          int hum=-1;
          getJsonTempHum(response_buffer, temp, &hum);
          printf("%.2lf %.2lf %.2lf %d\n\n", temp[0], temp[1], temp[2], hum);
        }
        free(response_buffer); response_buffer=NULL;
        response_size = 0;
      }
      //https_request();
    }
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
    
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    wifi_start();

    xTaskCreate(&RealizaHTTPRequest,  "Processa HTTP", 4096, NULL, 1, NULL);


    
}
