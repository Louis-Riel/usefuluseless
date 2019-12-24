#ifndef __tasker_h
#define __tasker_h

#include "esp_bt.h"
#include "../btc/btc.h"
#include "../wifi/station.h"

void taskerSallyForth(void *pvParameter);

typedef	struct {
                wifi_config*                    wtc;
                bt_config*                      btc;
	} app_config;

#endif