#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include "esp_log.h"

#include "tasker.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../btc/btc.h"
#include "../wifi/station.h"
#include "../rest/rest.h"
#include "../a4899/a4899.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static const char* TAG = "tasker";
static app_config cfg;


static void GetNvs(const char* partName, const char* paramName, void* blob,uint32_t sz) {
    ESP_LOGD(TAG,"%s Opening %s",__func__,partName);
    nvs_handle_t my_handle;
    esp_err_t err;
    err = nvs_open(partName, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s Failed to open %s", __func__,partName);
    } else {
        // Read the size of memory space required for blob
        ESP_LOGD(TAG,"%s Opening blob",__func__);
        size_t required_size = 0;  // value will default to 0, if not set yet in NVS
        err = nvs_get_blob(my_handle, paramName, NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG,"%s Failed to get blob %s", __func__,paramName);
        } else {
            if (required_size > 0 ) {
                ESP_LOGD(TAG,"%s reading blob %s %d",__func__,paramName, required_size);
                err = nvs_get_blob(my_handle, paramName, blob, &required_size);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG,"%s Failed to get blob %s",__func__,paramName);
                }
            } else {
                ESP_LOGD(TAG,"%s First config save %s",__func__,paramName);
                err = nvs_set_blob(my_handle, paramName, blob, sz);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG,"%s Failed to set blob %s %d",__func__,paramName, err);
                } else {
                    err = nvs_commit(my_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG,"%s Failed to commit blob %s %d",__func__,paramName, err);
                    } else {
                        nvs_close(my_handle);
                    }
                }
            }
        }
    }
}


void taskerSallyForth(void *pvParameter) {
    cfg.btc = (bt_config*)malloc(sizeof(bt_config));
    memset(cfg.btc,0,sizeof(bt_config));
    cfg.wtc = (wifi_config*)malloc(sizeof(wifi_config));
    memset(cfg.wtc,0,sizeof(wifi_config));

    cfg.btc->workPeriod = 6;
    cfg.btc->scanPeriod = 5;

    cfg.wtc->workPeriod = 11;
    cfg.wtc->scanPeriod = 4000;
    cfg.wtc->disconnectWaitTime = 2000;
    cfg.wtc->poolWaitTime = 3000;
    cfg.wtc->wifi_mode = WIFI_MODE_AP;
    strcpy((char*)cfg.wtc->wname, "utileinutile");
    strcpy((char*)cfg.wtc->wpdw, "sansfonctions");
    ESP_LOGD(TAG,"%s %s---%d",__func__,"wifi_workperiod",cfg.wtc->workPeriod);

    GetNvs("wnvs","wifi_config",cfg.wtc,sizeof(wifi_config));
    GetNvs("bnvs","bt_config",cfg.btc,sizeof(bt_config));
    ESP_LOGD(TAG,"%s %s---%d",__func__,"wifi_workperiod",cfg.wtc->workPeriod);

    cfg.btc->s_bt_eg = xEventGroupCreate();
    cfg.wtc->s_wifi_eg = xEventGroupCreate();
    cfg.wtc->s_bt_eg = cfg.btc->s_bt_eg;
    cfg.btc->s_wifi_eg = cfg.wtc->s_wifi_eg;

    //xTaskCreate(&wifiSallyForth, "wifiSallyForth", 4096, cfg.wtc , 5, NULL);
    //if (cfg.wtc->wifi_mode != WIFI_MODE_AP)
    //    xTaskCreate(&btSallyForth, "btSallyforth", 4096, cfg.btc, 5, NULL);

    //xTaskCreate(&restSallyForth, "restSallyForth", 4096, &cfg, 5, NULL);

    a4899* drv = new a4899(21,18);

    vTaskDelete( NULL );
}