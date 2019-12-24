#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef
	struct {
        uint32_t            workPeriod;
        uint32_t            scanPeriod;
        EventGroupHandle_t  s_bt_eg;
        EventGroupHandle_t  s_wifi_eg;
	}
updater_config;

void updaterSallyForth(void *pvParameter);
