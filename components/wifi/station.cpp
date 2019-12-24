#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "freertos/semphr.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h"

#include "station.h"
#include "../btc/btc.h"


#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static const char* TAG = "station";
uint8_t s_retry_num=0;
static wifi_config* config;
static bool wifiActive=false;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        uint32_t waitTime=0;
        if (xEventGroupGetBits(config->s_wifi_eg) | WIFI_CONNECTED_BIT) {
            waitTime=config->disconnectWaitTime;
            ESP_LOGI(TAG, "Got disconnected from AP");
        } else if (s_retry_num++ < 10) {
            waitTime=config->poolWaitTime;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGI(TAG, "Failed too many times, taking a longer break");
            waitTime=60000;
        }
        xEventGroupClearBits(config->s_wifi_eg, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(config->s_wifi_eg, WIFI_SCAN_READY_BIT);
        vTaskDelay(pdMS_TO_TICKS(waitTime));
        xEventGroupClearBits(config->s_wifi_eg, WIFI_SCAN_READY_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        vTaskDelay(2000/portTICK_PERIOD_MS);

        s_retry_num = 0;
        xEventGroupSetBits(config->s_wifi_eg, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(config->s_wifi_eg, WIFI_SCAN_READY_BIT);
    }
}

esp_err_t initWifi(){
    esp_err_t ret;

    ESP_LOGD(TAG,"%s Initializing netif",__func__);

    esp_netif_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_config_t wifi_config;
    memset(&wifi_config,0,sizeof(wifi_config_t));

    if (config->wifi_mode == WIFI_MODE_AP) {
        ESP_LOGD(TAG,"%s Configured in AP Mode %s",__func__,config->wname);
        esp_netif_create_default_wifi_ap();
        strcpy((char*)&wifi_config.ap.ssid[0],&config->wname[0]);
        strcpy((char*)&wifi_config.ap.password[0],&config->wpdw[0]);
        wifi_config.ap.max_connection=4;
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    } else {
        ESP_LOGD(TAG,"%s Configured in Station Mode %s",__func__,config->wname);
        esp_netif_create_default_wifi_sta();
        strcpy((char*)&wifi_config.sta.ssid[0],&config->wname[0]);
        strcpy((char*)&wifi_config.sta.password[0],&config->wpdw[0]);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(config->wifi_mode) );
    xEventGroupClearBits(config->s_wifi_eg,WIFI_SCAN_READY_BIT);
    if (config->wifi_mode == WIFI_MODE_AP) {
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    }
    ESP_ERROR_CHECK(esp_wifi_start() );

    xEventGroupSetBits(config->s_wifi_eg,WIFI_UP_BIT);

    ESP_LOGD(TAG, "wifi_init_sta finished.");

    if (config->wifi_mode == WIFI_MODE_AP) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        s_retry_num = 0;
        xEventGroupSetBits(config->s_wifi_eg, WIFI_CONNECTED_BIT);

    }
    return ESP_OK;
}

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    default:
        ESP_LOGD(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGD(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGD(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

void wifiScan() {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    wifi_scan_config_t scan_config = {
	.ssid = 0,
	.bssid = 0,
	.channel = 0,
    .show_hidden = true
    };

    TickType_t xLastWakeTime=xTaskGetTickCount();
    while(xEventGroupWaitBits(config->s_wifi_eg,WIFI_SCAN_READY_BIT,pdFALSE,pdFALSE,portMAX_DELAY) &&
          xEventGroupWaitBits(config->s_bt_eg,BT_SCAN_READY_BIT,pdFALSE,pdFALSE,portMAX_DELAY)){
        ESP_LOGD(TAG, "Scanning APs");
        xEventGroupClearBits(config->s_wifi_eg,WIFI_SCAN_READY_BIT);
        xEventGroupSetBits(config->s_wifi_eg,WIFI_SCANING_BIT);
        if (esp_wifi_scan_start(&scan_config, true) != ESP_OK) {
            vTaskDelay(100/portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGD(TAG, "Getting Scanned AP");
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGD(TAG, "Total APs scanned = %u", ap_count);
        for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
            ESP_LOGD(TAG, "SSID \t\t%s", ap_info[i].ssid);
            ESP_LOGD(TAG, "RSSI \t\t%d", ap_info[i].rssi);
            print_auth_mode(ap_info[i].authmode);
            if (ap_info[i].authmode != WIFI_AUTH_WEP) {
                print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
            }
            ESP_LOGD(TAG, "Channel \t\t%d\n", ap_info[i].primary);
        }

        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        xEventGroupSetBits(config->s_wifi_eg,WIFI_SCAN_READY_BIT);
        xEventGroupClearBits(config->s_wifi_eg,WIFI_SCANING_BIT);
        ESP_LOGD(TAG,"%d >= %d",xTaskGetTickCount()-xLastWakeTime , pdMS_TO_TICKS(config->scanPeriod ));
        if ( (xTaskGetTickCount()-xLastWakeTime) >= pdMS_TO_TICKS(config->scanPeriod )) {
            ESP_LOGD(TAG, "Done Scanned AP");
            break;
        }
    }
}

void wifiSallyForth(void *pvParameter) {
    if (pvParameter != NULL) {
        config = (wifi_config*)pvParameter;
    } else {
        ESP_LOGE(TAG,"%s Missing config",__func__);
        vTaskDelete( NULL );
    }

    ESP_LOGD(TAG,"%s initing wifi",__func__);
    if (initWifi()) {
        ESP_LOGE(TAG, "%s wifi init failed", __func__);
    }
    wifiActive=true;
    //wifiScan(pvParameter);
    TickType_t xLastWakeTime;
    while(wifiActive){
        xLastWakeTime= xTaskGetTickCount();
	    //wifiScan();
        ESP_LOGD(TAG,"Sleepee");
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( config->workPeriod*1000 ));
        ESP_LOGD(TAG,"Wakey");
    }

    vTaskDelete( NULL );
}
