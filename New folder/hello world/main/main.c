#include <stdio.h>
#include <esp_log.h>

char * TAG ="HELLO WORLD";

void app_main(void)
{
    ESP_LOGI(TAG," INFORMATION API");
    ESP_LOGW(TAG," warning API");
    ESP_LOGE(TAG,"  error API");
    ESP_LOGD(TAG,"  debag API");
    ESP_LOGV(TAG,"  verbose API");
    
}