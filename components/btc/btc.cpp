#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "math.h"

#include "btc.h"
#include "../wifi/station.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_gap_bt_api.h"
#include <esp_spp_api.h>
#include "freertos/semphr.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "freertos/event_groups.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "time.h"
#include "sys/time.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#define MAX_BT_POOL_SIZE 500

const uint32_t BT_DEV_SIZE=sizeof(bt_device);
const uint32_t BT_POOL_SIZE=sizeof(bt_device)*MAX_BT_POOL_SIZE;
const char skippedChar[] = {' ','/'};
const int skippedCharCnt = 2;

static const char* TAG = "btc";
static bt_device* btLivePool = (bt_device*)malloc( MAX_BT_POOL_SIZE * BT_DEV_SIZE); //4319580...4315840...4315840...4319580...
static int btPoolLen=0;
static uint64_t scanTs=0;
static bt_config* config;
static int btWeirdnessIndex=0;
static bool btactive=true;

static struct timeval time_new, time_old;
static long data_num = 0;

extern void stopBt(){
    btactive=false;
}

static int isInPool(esp_bd_addr_t dev){
    if (btPoolLen > 0){
        for (int idx=0; idx < MAX_BT_POOL_SIZE; idx++){
            bt_device* p = &btLivePool[idx];
            if ((p->baddr[0] == dev[0]) &&
                (p->baddr[1] == dev[1]) &&
                (p->baddr[2] == dev[2]) &&
                (p->baddr[3] == dev[3]) &&
                (p->baddr[4] == dev[4]) &&
                (p->baddr[5] == dev[5])) {
                    return idx;
            }
        }
    }
    return -1;
}

static int addToPool(esp_bd_addr_t dev){
    int devIdx=isInPool(dev);
    if ((devIdx==-1) && (btPoolLen<MAX_BT_POOL_SIZE)){
        bt_device *p = (bt_device*)&btLivePool[btPoolLen++];
        p->baddr[0] = dev[0];
        p->baddr[1] = dev[1];
        p->baddr[2] = dev[2];
        p->baddr[3] = dev[3];
        p->baddr[4] = dev[4];
        p->baddr[5] = dev[5];
        p->name[0]=0;
        p->rssi=-128;
        p->cod=-1;
        p->bootId=0;
        p->lastTs=esp_timer_get_time();
        p->createTs=p->lastTs;
        p->totTs=0;
        p->present=true;
        p->goneCnt=0;
        return btPoolLen;
    }
    return devIdx;
}

static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

static void print_speed(void)
{
    float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
    float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
    float time_interval = time_new_s - time_old_s;
    float speed = data_num * 8 / time_interval / 1000.0;
    ESP_LOGD(TAG, "speed(%fs ~ %fs): %f kbit/s" , time_old_s, time_new_s, speed);
    data_num = 0;
    time_old.tv_sec = time_new.tv_sec;
    time_old.tv_usec = time_new.tv_usec;
}
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
   esp_err_t ret = ESP_OK;
   switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGD(TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name("IPhone");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        ret = esp_spp_start_srv(ESP_SPP_SEC_AUTHORIZE,ESP_SPP_ROLE_MASTER,0,"IWatch");
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_spp_start_srv failed %d",ret);
        }
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(TAG, "ESP_SPP_DISCOVERY_COMP_EVT status=%d scn_num=%d",param->disc_comp.status, param->disc_comp.scn_num);
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
        ESP_LOGI(TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                 param->data_ind.len, param->data_ind.handle);
        esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
#else
        gettimeofday(&time_new, NULL);
        data_num += param->data_ind.len;
        if (time_new.tv_sec - time_old.tv_sec >= 3) {
            print_speed();
        }
#endif
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT");
        gettimeofday(&time_old, NULL);
        break;
    default:
        break;
    }
}

static void update_device_info(esp_bt_gap_cb_param_t *param)
{
    char bda_str[18];
    uint32_t cod = 0;
    int32_t rssi = -129; /* invalid value */
    esp_bt_gap_dev_prop_t *p;
    int devIdx=isInPool(param->disc_res.bda);

    if (devIdx == -1){
        devIdx=addToPool(param->disc_res.bda);
        ESP_LOGI(TAG, "New device found: %s", bda2str(param->disc_res.bda, bda_str, 18));
        if (esp_bt_gap_read_remote_name(param->disc_res.bda)!=ESP_OK){
            ESP_LOGE(TAG,"esp_bt_gap_read_remote_name failed");
        }
        esp_spp_start_discovery(param->disc_res.bda);
    }

    if (devIdx == -1) {
        ESP_LOGE(TAG,"Cannot allocate for device");
        return;
    }
    bt_device* dev = &btLivePool[devIdx];
    if (dev->present)
        dev->totTs+=(esp_timer_get_time()-dev->lastTs);
    dev->lastTs=esp_timer_get_time();

    for (int i = 0; i < param->disc_res.num_prop; i++) {
        p = param->disc_res.prop + i;
        switch (p->type) {
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *(uint32_t *)(p->val);
            if (dev->cod != cod) {
                dev->cod = cod;
            }
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *(int8_t *)(p->val);
            if (dev->rssi != rssi) {
                dev->rssi = rssi;
            }
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
            ESP_LOGI(TAG, "Name: %s", (char*)p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_EIR:
            if (strlen(dev->name)==0) {
                char* name= (char*)p->val;
                int len=fmin(strlen(name),sizeof(dev->name));
                bool wasSkippedChar=false;
                int curIdx=0;
                bool headingSpaces=false;
                char curChar=0;
                bool isASkippedChar=false;
                for (int idx = 0; idx < len; idx++) {
                    curChar=name[idx];
                    if (idx == 0) {
                        headingSpaces=true;
                    }

                    isASkippedChar=false;
                    for (int idx2=0; idx2<skippedCharCnt; idx2++) {
                        if ((name[idx]==skippedChar[idx2])) {
                            isASkippedChar=true;
                            break;
                        }
                    }

                    if (name[idx] < 32) {
                        continue;
                    }

                    if (headingSpaces){
                        headingSpaces=isASkippedChar;
                        if (headingSpaces) {
                            continue;
                        }
                    }

                    if (isASkippedChar && wasSkippedChar) {
                        continue;
                    }

                    dev->name[curIdx++]=curChar;
                    wasSkippedChar=isASkippedChar;
                }
                dev->name[curIdx]=0;
                ESP_LOGI(TAG, "NAME: %s", dev->name);
            }
            break;
        default:
            break;
        }
    }
}

static int on_bthci_receive (uint8_t *data, uint16_t len)
{
    ESP_LOGD(TAG,"Can got %d bytes",len);
    return 0;
}

static void on_bthci_can_send (void)
{
    ESP_LOGD(TAG,"Can Send");
}


static const esp_vhci_host_callback_t vhci_callback = {
    .notify_host_send_available = on_bthci_can_send,
    .notify_host_recv = on_bthci_receive,
};

static void esp_ble_cb(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    char bda_str[18];
    switch(event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGD(TAG, "ADV DATA Set");
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGD(TAG, "ADV RSP DATA Set");
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Advertising start failed");
        }
        else {
            ESP_LOGI(TAG, "Start adv successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Advertising stop failed");
        }
        else {
            ESP_LOGI(TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                param->update_conn_params.status,
                param->update_conn_params.min_int,
                param->update_conn_params.max_int,
                param->update_conn_params.conn_int,
                param->update_conn_params.latency,
                param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            int devIdx=isInPool(param->scan_rst.bda);

            if (devIdx == -1){
                devIdx=addToPool(param->scan_rst.bda);
                if( ESP_OK != esp_bt_gap_read_remote_name(param->scan_rst.bda)) {
                    ESP_LOGE(TAG,"esp_bt_gap_read_remote_name failed");
                }
                uint8_t nameLen=0;
                for (uint8_t idx=ESP_BLE_AD_TYPE_FLAG; idx <= ESP_BLE_AD_TYPE_128SERVICE_DATA; idx++){
                    uint8_t* nname = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, idx, &nameLen);
                    if (nameLen > 0) {
                        ESP_LOGD(TAG, "BLE prop: %d %s",idx,  nname);
                        if (idx == ESP_BLE_AD_TYPE_SERVICE_DATA) {
                            ESP_LOGD(TAG, "BLE prop: %d %s",idx,  &nname[2]);
                            for (uint8_t* ch=nname; ch <= nname+nameLen; ch++ ) {
                                ESP_LOGD(TAG,"(%d)",*ch);
                            }
                        }
                        if (idx == ESP_BLE_AD_TYPE_NAME_CMPL) {
                            ESP_LOGD(TAG, "Name: %d %s",nameLen,  nname);
                            strcpy((char*)nname,btLivePool[devIdx].name);
                        }
                    }
                }
                ESP_LOGI(TAG, "New BLE device found: %s %d", bda2str(param->scan_rst.bda, bda_str, 18),param->scan_rst.ble_addr_type);
                ESP_LOGI(TAG, "RAM:%d...", esp_get_free_heap_size());
            }

            if (devIdx == -1) {
                ESP_LOGE(TAG,"Cannot allocate for device");
                return;
            }
            bt_device* dev = &btLivePool[devIdx];
            if (dev->present)
                dev->totTs+=(esp_timer_get_time()-dev->lastTs);
            dev->lastTs=esp_timer_get_time();
            dev->rssi=param->scan_rst.rssi;
            dev->present=true;
            dev->goneCnt=0;
        }
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        ESP_LOGD(TAG, "BLE Discovery started.");
        break;
    default:
        ESP_LOGI(TAG,"Unhandled GAP event: %d", event);
        break;
    }
}

static void gap_callback (esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    char bda_str[18];
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        update_device_info(param);
        break;
    }

//    case ESP_BT_GAP_DEV_PROP_BDNAME:
//        ESP_LOGI(TAG, "BDNAME: %s", (char *)p->val);
//        break;

    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            if (xEventGroupGetBits(config->s_bt_eg) & BT_SCANING_BIT) {
                esp_err_t ret;
                ESP_LOGD(TAG, "Device discovery stopped.");

                if ((ret = esp_bt_gap_cancel_discovery())) {
                    ESP_LOGE(TAG, "%s bluetooth cancel discovery failed %d", __func__, ret);
                }
                for (int idx=0; idx < btPoolLen; idx++) {
                    bt_device* dev = &btLivePool[idx];
                    if (dev->present && (dev->lastTs < scanTs)) {
                        dev->goneCnt++;
                        dev->present=dev->goneCnt<=3;
                        if (!dev->present) {
                            ESP_LOGD(TAG, "RAM:%d...", esp_get_free_heap_size());
                            ESP_LOGD(TAG,"%s %s Left",bda2str(dev->baddr, bda_str, 18), dev->name);
                        }
                    }
                    if (!dev->present && (dev->lastTs >= scanTs)) {
                        dev->present=true;
                        dev->goneCnt=0;
                        ESP_LOGD(TAG, "RAM:%d...", esp_get_free_heap_size());
                        ESP_LOGD(TAG,"%s %s Cameback",bda2str(dev->baddr, bda_str, 18), dev->name);
                    }
                }

                uint64_t uptick = esp_timer_get_time();
                uint16_t uptime = uptick/1000000;
                uint8_t useconds=uptime%60;
                uint8_t uminutes=((uptime-useconds)/60)%60;

                ESP_LOGD(TAG, "RAM:%d Time %02d:%02d:%02d",
                        esp_get_free_heap_size(),
                        (int)(((uptime-useconds)/60)-uminutes)/60,
                        uminutes,
                        useconds);
                ESP_LOGD(TAG," ___________________________________________________________________________________");
                ESP_LOGD(TAG,"|     BADDR        |  COD   |  Since |  Total |  Last  |  RSSI  |  Last  | Lastts  |");
                ESP_LOGD(TAG," ___________________________________________________________________________________");
                for (int idx=0; idx < btPoolLen; idx++) {
                    bt_device* dev = &btLivePool[idx];
                    uint16_t cts = (uptick-dev->createTs)/1000000;
                    uint16_t pts = (uptick-dev->lastTs)/1000000;
                    uint16_t ttts = (dev->lastTs)/1000000;
                    uint16_t tts = dev->totTs/1000000;
                    uint8_t cseconds=cts%60;
                    uint8_t cminutes=((cts-cseconds)/60)%60;
                    uint8_t pseconds=pts%60;
                    uint8_t pminutes=((pts-pseconds)/60)%60;
                    uint8_t tseconds=tts%60;
                    uint8_t tminutes=((tts-tseconds)/60)%60;
                    uint8_t ttseconds=ttts%60;
                    uint8_t ttminutes=((ttts-ttseconds)/60)%60;

                    ESP_LOGD(TAG,"|%s%s|%08x|%02d:%02d:%02d|%02d:%02d:%02d|%02d:%02d:%02d|%8d|%8d|%02d:%02d:%02d|",
                        dev->present?"*":" ",
                        bda2str(dev->baddr, bda_str, 18),
                        dev->cod,
                        (int)(((cts-cseconds)/60)-cminutes)/60,
                        cminutes,
                        cseconds,
                        (int)(((tts-tseconds)/60)-tminutes)/60,
                        tminutes,
                        tseconds,
                        (int)(((pts-pseconds)/60)-pminutes)/60,
                        pminutes,
                        pseconds,
                        dev->rssi,
                        dev->goneCnt,
                        (int)(((ttts-ttseconds)/60)-ttminutes)/60,
                        ttminutes,
                        ttseconds);
                }
                ESP_LOGD(TAG," ___________________________________________________________________________________");

                xEventGroupSetBits(config->s_bt_eg,BT_SCAN_READY_BIT);
                xEventGroupClearBits(config->s_bt_eg,BT_SCANING_BIT);
            }
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            if (xEventGroupGetBits(config->s_bt_eg) & BT_SCAN_READY_BIT) {
                xEventGroupClearBits(config->s_bt_eg,BT_SCAN_READY_BIT);
                xEventGroupSetBits(config->s_bt_eg,BT_SCANING_BIT);
                ESP_LOGD(TAG, "Discovery started.");
            }
        } else {
            ESP_LOGD(TAG, "%s Other GAP state change:%d.", __func__, param->disc_st_chg.state);
        }
        break;
    }
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGD(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }
#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif
    default: {
        ESP_LOGD(TAG, "event: %d", event);
        break;
    }
    }
}

void btScan() {
    if (!config->s_bt_eg) {
        ESP_LOGD(TAG, "%s No config, getting outta dodge", __func__);
        btWeirdnessIndex+=100;
        return ;
    }

    scanTs = esp_timer_get_time();

    esp_ble_gap_start_scanning(5);

    ESP_LOGD(TAG, "%s BLE scanning", __func__);

    if (ESP_OK != esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE,ESP_BT_NON_DISCOVERABLE)){
        ESP_LOGE(TAG,"esp_bt_gap_set_scan_mode failed");
        return;
    }

    esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY,config->scanPeriod,0);

    if (ret != ESP_OK) {
        btWeirdnessIndex+=3;
        ESP_LOGD(TAG, "%s bluetooth scanning failed", __func__);
    } else {
        ESP_LOGD(TAG, "%s bluetooth scanning", __func__);
    }
}

extern "C" {
    static esp_err_t external_storage_mac_get(uint8_t *mac);
}

static esp_err_t external_storage_mac_get(uint8_t *mac)
{
    if (mac == NULL) {
        ESP_LOGE(TAG, "The mac parameter is NULL");
        abort();
    }
    uint8_t p[6] = {0};
    esp_fill_random(p,6);

    uint32_t tidx = 59 * (((float)esp_random())/UINT32_MAX);

    memcpy(p, apoui[tidx] ,3);

    memcpy(mac, p, 6);
    ESP_LOGD(TAG, "MMMMAC:%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);

    return ESP_OK;
}

esp_err_t deinitBTC() {
    esp_err_t ret = esp_spp_deinit();
    if (ret == ESP_OK)
        ESP_LOGD(TAG, "%s spp disabled", __func__);

    ret = esp_bluedroid_disable();
    if (ret == ESP_OK)
        ESP_LOGD(TAG, "%s bluetooth disabled", __func__);

    ret = esp_bluedroid_deinit();
    if (ret == ESP_OK)
        ESP_LOGD(TAG, "%s deinited bluetooth", __func__);

    ret=esp_bt_controller_disable();
    if (ret == ESP_OK)
        ESP_LOGD(TAG, "%s controller down", __func__);
    return ESP_OK;
}


esp_err_t initBTC() {
    if (config->s_bt_eg && xEventGroupGetBits(config->s_bt_eg) & BT_UP_BIT) {
        return ESP_OK;
    }
    btWeirdnessIndex=0;
    uint8_t base_mac_addr[6] = {0};
    //esp_err_t ret = external_storage_mac_get(base_mac_addr);
    //if (ret != ESP_OK) {
    //    ESP_LOGE(TAG, "Failed to get base MAC address from external storage. (%s)", esp_err_to_name(ret));
    //    ESP_LOGE(TAG, "Aborting");
    //    abort();
    //} else {
    //    ESP_LOGI(TAG, "Base MAC Address read from external storage");
    //}
//
    //esp_base_mac_addr_set(base_mac_addr);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_LOGD(TAG, "%s initialized controller", __func__);

    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
    ESP_LOGD(TAG, "%s enabled controller", __func__);

    ESP_ERROR_CHECK( esp_vhci_host_register_callback(&vhci_callback));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_LOGD(TAG, "%s inited bluetooth", __func__);

    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_LOGD(TAG, "%s bluetooth enabled", __func__);

    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_init(ESP_SPP_MODE_CB));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_ble_cb));

    ESP_ERROR_CHECK(esp_bt_gap_register_callback (gap_callback));
    ESP_LOGD(TAG, "%s callback registered", __func__);

    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
#if (CONFIG_BT_SSP_ENABLED == true)
    ESP_ERROR_CHECK(esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t)));
#endif
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(pin_type, 0, pin_code));


    xEventGroupSetBits(config->s_bt_eg,BT_UP_BIT);
    xEventGroupSetBits(config->s_bt_eg,BLE_UP_BIT);
    xEventGroupSetBits(config->s_bt_eg,BT_SCAN_READY_BIT);

    return ESP_OK;
}

void btSallyForth(void *pvParameter) {

    if (pvParameter != NULL) {
        config = (bt_config*)pvParameter;
        ESP_LOGD(TAG,"%s workperiod:%d",__func__,config->workPeriod);
    } else {
        ESP_LOGE(TAG,"%s No params", __func__);
        return;
    }

    btactive=true;

    if (initBTC()) {
        ESP_LOGE(TAG, "%s bluetooth init failed", __func__);
        btWeirdnessIndex+=100;
    }

    ESP_LOGD(TAG, "%s BT Gogo gadget", __func__);
    TickType_t xLastWakeTime;
    int lastWeirness = btWeirdnessIndex;
    while(btactive){
        xLastWakeTime= xTaskGetTickCount();
        //xEventGroupWaitBits(config->s_wifi_eg,WIFI_SCAN_READY_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
        xEventGroupWaitBits(config->s_bt_eg,BT_SCAN_READY_BIT,pdFALSE,pdFALSE,pdMS_TO_TICKS(config->workPeriod*1050));
        if (config->scanPeriod>0) {
            btScan();
            if (!(xEventGroupWaitBits(config->s_bt_eg,BT_SCANING_BIT,pdFALSE,pdFALSE,pdMS_TO_TICKS(config->scanPeriod*1300)) & BT_SCANING_BIT)) {
                ESP_LOGE(TAG,"Timed out waiting on BT to stop scanning");
                btWeirdnessIndex+=10;
            }
            ESP_LOGD(TAG,"Sleepee");
            if (lastWeirness != btWeirdnessIndex) {
                lastWeirness = btWeirdnessIndex;
                ESP_LOGD(TAG,"%s bt weirdness %d",__func__,btWeirdnessIndex);
                if (btWeirdnessIndex >= 100) {
                    deinitBTC();
                    initBTC();
                }
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( config->workPeriod*1000 ));
        ESP_LOGD(TAG,"Wakey");
    }

    esp_bt_gap_cancel_discovery();
    esp_spp_deinit();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();

    vTaskDelete( NULL );
}

uint32_t btToJSON(char* buf){
    char bda_str[18];
    uint32_t len=sprintf(buf,"{\"devices\":[\n");
    for (int idx=0; idx < btPoolLen; idx++) {
        bt_device* dev = &btLivePool[idx];
        uint16_t cts = (esp_timer_get_time()-dev->createTs)/1000000;
        uint16_t pts = (esp_timer_get_time()-dev->lastTs)/1000000;
        uint16_t tts = dev->totTs/1000000;
        uint8_t cseconds=cts%60;
        uint8_t cminutes=((cts-cseconds)/60)%60;
        uint8_t pseconds=pts%60;
        uint8_t pminutes=((pts-pseconds)/60)%60;
        uint8_t tseconds=tts%60;
        uint8_t tminutes=((tts-tseconds)/60)%60;

        len+=sprintf(&buf[len],"  {\"present\":%s,\"baddr\":\"%s\",\"name\":\"%s\",\"cod\":\"%08x\",\"since\":\"%02d:%02d:%02d\",\"spent\":\"%02d:%02d:%02d\",\"last\":\"%02d:%02d:%02d\",\"rssi\":%d}%s",
                        dev->present?"true":"false",
                        bda2str(dev->baddr, bda_str, 18),
                        dev->name,
                        dev->cod,
                        (int)(((cts-cseconds)/60)-cminutes)/60,
                        cminutes,
                        cseconds,
                        (int)(((tts-tseconds)/60)-tminutes)/60,
                        tminutes,
                        tseconds,
                        (int)(((pts-pseconds)/60)-pminutes)/60,
                        pminutes,
                        pseconds,
                        dev->rssi,
                        idx < (btPoolLen-1)?",\n":"\n");
    }
    len+=sprintf(&buf[len],"]}");
    return len;
}
