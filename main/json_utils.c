#include "json_utils.h"

#include "cJSON.h"

void getJsonData(char * response_buffer, double * lat, double * lng){
    cJSON * json = cJSON_Parse (response_buffer);
    *lat = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
    *lng = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;
    cJSON_Delete(json);
}