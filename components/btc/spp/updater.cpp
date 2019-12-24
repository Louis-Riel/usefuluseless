#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "nvs_flash.h"

#include "../../wifi/station.h"
#include "../btc.h"
#include "updater.h"
#include "driver/uart.h"

static updater_config config;

void updaterSallyForth(void *pvParameter) {

    if (pvParameter != NULL) {
        updater_config* pc = (updater_config*)pvParameter;
        config.scanPeriod=pc->scanPeriod?pc->scanPeriod:10;
        config.workPeriod=pc->workPeriod?pc->workPeriod:15;
        config.s_bt_eg=pc->s_bt_eg;
        config.s_wifi_eg=pc->s_wifi_eg;
    }

    xEventGroupWaitBits(config.s_bt_eg,BT_UP_BIT,pdFALSE,pdFALSE,portMAX_DELAY);

    ESP_LOGD(TAG, "%s BT Up, Standin up SPP", __func__);

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_RTS,
        .rx_flow_ctrl_thresh = 122,
    };

    uart_param_config(UART_NUM_0, &uart_config);
    
    TickType_t xLastWakeTime;
    while(true){
        xLastWakeTime= xTaskGetTickCount();
        //xEventGroupWaitBits(config.s_wifi_eg,WIFI_SCAN_READY_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
        xEventGroupWaitBits(config.s_bt_eg,BT_SCAN_READY_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
	    btScan();
        xEventGroupWaitBits(config.s_bt_eg,BT_SCANING_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
        ESP_LOGI(TAG,"Sleepee");
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( config.workPeriod*1000 ));
        ESP_LOGI(TAG,"Wakey");
    }


    vTaskDelete( NULL );
}