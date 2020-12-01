#include "json_utils.h"

#include "cJSON.h"

void getJsonLatLng(char * buffer, double * lat, double * lng){
    cJSON * json = cJSON_Parse (buffer);

    *lat = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
    *lng = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;

    cJSON_Delete(json);
}

void getJsonTempHum(char * buffer, double temp[], int * hum){
    cJSON * json = cJSON_Parse (buffer);

    cJSON * main_json = cJSON_GetObjectItemCaseSensitive(json, "main");

    temp[0] = cJSON_GetObjectItemCaseSensitive(main_json, "temp")->valuedouble;
    temp[1] = cJSON_GetObjectItemCaseSensitive(main_json, "temp_min")->valuedouble;
    temp[2] = cJSON_GetObjectItemCaseSensitive(main_json, "temp_max")->valuedouble;

    *hum = cJSON_GetObjectItemCaseSensitive(main_json, "humidity")->valueint;

    cJSON_Delete(json);
}