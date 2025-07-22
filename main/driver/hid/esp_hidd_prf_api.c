#include "esp_hidd_prf_api.h"

#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "ui/events/events.h"

#define HID_DEMO_TAG "BLE HID"

#define DIAL_UNIT 100
uint8_t ble_connect_num = 0; // 蓝牙连接数量
// 获取蓝牙连接状态
bool get_ble_connection_status(void)
{
    return sec_conn;
}
void esp_hidd_prf_cb_hdl(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_DIAL_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
#define PROFILE_NUM 2
#define PROFILE_APP_BAT 0
#define PROFILE_APP_HID 1
#define PROFILE_APP_DIAL 2
static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_BAT] = {
        .gatts_cb = esp_hidd_prf_cb_hdl,
        .gatts_if = ESP_GATT_IF_NONE,
        .UUID = BATTRAY_APP_ID,
    },
    [PROFILE_APP_HID] = {
        .gatts_cb = esp_hidd_prf_cb_hdl,
        .gatts_if = ESP_GATT_IF_NONE,
        .UUID = HIDD_APP_ID,
    },
    // [PROFILE_APP_DIAL] = {
    //     .gatts_cb = gatts_DIAL_event_handler,
    //     .gatts_if = ESP_GATT_IF_NONE,
    //     .UUID = DIAL_APP_ID,
    // },
};

/// characteristic presentation information
struct prf_char_pres_fmt
{
    /// Unit (The Unit is a UUID)
    uint16_t unit;
    /// Description
    uint16_t description;
    /// Format
    uint8_t format;
    /// Exponent
    uint8_t exponent;
    /// Name space
    uint8_t name_space;
};

// HID report mapping table
static hid_report_map_t hid_rpt_map[HID_NUM_REPORTS];

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

hidd_le_env_t hidd_le_env;

// HID报告映射长度
uint8_t hidReportMapLen = sizeof(hidReportMap);
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;

// HID report mapping table
// static hidRptMap_t  hidRptMap[HID_NUM_REPORTS];

// HID信息特征值
static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version)
    0x00,                                 // bCountryCode
    HID_KBD_FLAGS                         // Flags
};

// HID External Report Reference Descriptor
static uint16_t hidExtReportRefDesc = ESP_GATT_UUID_BATTERY_LEVEL;

// HID Report Reference characteristic descriptor, mouse input
static uint8_t hidReportRefMouseIn[HID_REPORT_REF_LEN] =
    {HID_ITF_PROTOCOL_MOUSE, HID_REPORT_TYPE_INPUT};

// HID Report Reference characteristic descriptor, key input
static uint8_t hidReportRefKeyIn[HID_REPORT_REF_LEN] =
    {HID_ITF_PROTOCOL_KEYBOARD, HID_REPORT_TYPE_INPUT};

// HID Report Reference characteristic descriptor, LED output
static uint8_t hidReportRefLedOut[HID_REPORT_REF_LEN] =
    {HID_ITF_PROTOCOL_KEYBOARD, HID_REPORT_TYPE_OUTPUT};

#if (SUPPORT_REPORT_VENDOR == true)

static uint8_t hidReportRefVendorOut[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_VENDOR_OUT, HID_REPORT_TYPE_OUTPUT};
#endif

// HID Report Reference characteristic descriptor, Feature
static uint8_t hidReportRefFeature[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_FEATURE, HID_REPORT_TYPE_FEATURE};

// HID Report Reference characteristic descriptor, consumer control input
static uint8_t hidReportRefCCIn[HID_REPORT_REF_LEN] =
    {HID_ITF_PROTOCOL_MEDIA, HID_REPORT_TYPE_INPUT};

static uint8_t hidReportRefDialIn[HID_REPORT_REF_LEN] =
    {HID_ITF_PROTOCOL_DIAL, HID_REPORT_TYPE_INPUT};

/*
 *  Heart Rate PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// hid Service uuid
static uint16_t hid_le_svc = HIDD_APP_ID;
uint16_t hid_count = 0;
esp_gatts_incl_svc_desc_t incl_svc = {0};

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
/// the uuid definition
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t include_service_uuid = ESP_GATT_UUID_INCLUDE_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint16_t hid_info_char_uuid = ESP_GATT_UUID_HID_INFORMATION;
static const uint16_t hid_report_map_uuid = ESP_GATT_UUID_HID_REPORT_MAP;
static const uint16_t hid_control_point_uuid = ESP_GATT_UUID_HID_CONTROL_POINT;
static const uint16_t hid_report_uuid = ESP_GATT_UUID_HID_REPORT;
static const uint16_t hid_proto_mode_uuid = ESP_GATT_UUID_HID_PROTO_MODE;
static const uint16_t hid_kb_input_uuid = ESP_GATT_UUID_HID_BT_KB_INPUT;
static const uint16_t hid_kb_output_uuid = ESP_GATT_UUID_HID_BT_KB_OUTPUT;
static const uint16_t hid_mouse_input_uuid = ESP_GATT_UUID_HID_BT_MOUSE_INPUT;
static const uint16_t hid_repot_map_ext_desc_uuid = ESP_GATT_UUID_EXT_RPT_REF_DESCR;
static const uint16_t hid_report_ref_descr_uuid = ESP_GATT_UUID_RPT_REF_DESCR;
/// the propoty definition
// static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write_nr = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
// static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_write_nr = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

/// battary Service
static const uint16_t battary_svc = ESP_GATT_UUID_BATTERY_SERVICE_SVC;

static const uint16_t bat_lev_uuid = ESP_GATT_UUID_BATTERY_LEVEL;
static const uint8_t bat_lev_ccc[2] = {0x00, 0x00};
static const uint16_t char_format_uuid = ESP_GATT_UUID_CHAR_PRESENT_FORMAT;

static uint8_t battary_lev = 99;
void set_battary_level(uint8_t level)
{
    battary_lev = level;
    ESP_LOGI("BAT", "set_battary_level %d", battary_lev);
}
/// Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t bas_att_db[BAS_IDX_NB] =
    {
        // Battary Service Declaration
        [BAS_IDX_SVC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(battary_svc), (uint8_t *)&battary_svc}},

        // Battary level Characteristic Declaration
        [BAS_IDX_BATT_LVL_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

        // Battary level Characteristic Value
        [BAS_IDX_BATT_LVL_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&bat_lev_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), &battary_lev}},

        // Battary level Characteristic - Client Characteristic Configuration Descriptor
        [BAS_IDX_BATT_LVL_NTF_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(bat_lev_ccc), (uint8_t *)bat_lev_ccc}},

        // Battary level report Characteristic Declaration
        [BAS_IDX_BATT_LVL_PRES_FMT] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_format_uuid, ESP_GATT_PERM_READ, sizeof(struct prf_char_pres_fmt), 0, NULL}},
};

/// Full Hid device Database Description - Used to add attributes into the database
static esp_gatts_attr_db_t hidd_le_gatt_db[HIDD_LE_IDX_NB] =
    {
        // HID Service Declaration
        [HIDD_LE_IDX_SVC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ_ENCRYPTED, sizeof(uint16_t), sizeof(hid_le_svc), (uint8_t *)&hid_le_svc}},

        // // HID Service Declaration
        [HIDD_LE_IDX_INCL_SVC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&include_service_uuid, ESP_GATT_PERM_READ, sizeof(esp_gatts_incl_svc_desc_t), sizeof(esp_gatts_incl_svc_desc_t), (uint8_t *)&incl_svc}},

        // HID Information Characteristic Declaration
        [HIDD_LE_IDX_HID_INFO_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
        // HID Information Characteristic Value
        [HIDD_LE_IDX_HID_INFO_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_info_char_uuid, ESP_GATT_PERM_READ, sizeof(hids_hid_info_t), sizeof(hidInfo), (uint8_t *)&hidInfo}},

        // HID Control Point Characteristic Declaration
        [HIDD_LE_IDX_HID_CTNL_PT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write_nr}},
        // HID Control Point Characteristic Value
        [HIDD_LE_IDX_HID_CTNL_PT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_control_point_uuid, ESP_GATT_PERM_WRITE, sizeof(uint8_t), 0, NULL}},

        // Report Map Characteristic Declaration
        [HIDD_LE_IDX_REPORT_MAP_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
        // Report Map Characteristic Value
        [HIDD_LE_IDX_REPORT_MAP_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_map_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAP_MAX_LEN, sizeof(hidReportMap), (uint8_t *)&hidReportMap}},

        // Report Map Characteristic - External Report Reference Descriptor
        [HIDD_LE_IDX_REPORT_MAP_EXT_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_repot_map_ext_desc_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&hidExtReportRefDesc}},

        // Protocol Mode Characteristic Declaration
        [HIDD_LE_IDX_PROTO_MODE_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
        // Protocol Mode Characteristic Value
        [HIDD_LE_IDX_PROTO_MODE_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_proto_mode_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint8_t), sizeof(hidProtocolMode), (uint8_t *)&hidProtocolMode}},

        [HIDD_LE_IDX_REPORT_MOUSE_IN_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

        [HIDD_LE_IDX_REPORT_MOUSE_IN_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},

        [HIDD_LE_IDX_REPORT_MOUSE_IN_CCC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}},

        [HIDD_LE_IDX_REPORT_MOUSE_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefMouseIn), sizeof(hidReportRefMouseIn), hidReportRefMouseIn}},
        // Report Characteristic Declaration
        [HIDD_LE_IDX_REPORT_KEY_IN_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        // Report Characteristic Value
        [HIDD_LE_IDX_REPORT_KEY_IN_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},
        // Report KEY INPUT Characteristic - Client Characteristic Configuration Descriptor
        [HIDD_LE_IDX_REPORT_KEY_IN_CCC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}},
        // Report Characteristic - Report Reference Descriptor
        [HIDD_LE_IDX_REPORT_KEY_IN_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefKeyIn), sizeof(hidReportRefKeyIn), hidReportRefKeyIn}},

        // Report Characteristic Declaration
        [HIDD_LE_IDX_REPORT_LED_OUT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_write_nr}},

        [HIDD_LE_IDX_REPORT_LED_OUT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},
        [HIDD_LE_IDX_REPORT_LED_OUT_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefLedOut), sizeof(hidReportRefLedOut), hidReportRefLedOut}},
        // Report Characteristic Declaration
        [HIDD_LE_IDX_REPORT_CC_IN_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        // Report Characteristic Value
        [HIDD_LE_IDX_REPORT_CC_IN_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},
        // Report KEY INPUT Characteristic - Client Characteristic Configuration Descriptor
        [HIDD_LE_IDX_REPORT_CC_IN_CCC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED), sizeof(uint16_t), 0, NULL}},
        // Report Characteristic - Report Reference Descriptor
        [HIDD_LE_IDX_REPORT_CC_IN_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefCCIn), sizeof(hidReportRefCCIn), hidReportRefCCIn}},

        // Report Characteristic Declaration for stylus IN
        [HIDD_LE_IDX_REPORT_DIAL_IN_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [HIDD_LE_IDX_REPORT_DIAL_IN_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},
        [HIDD_LE_IDX_REPORT_DIAL_IN_CCC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}},
        [HIDD_LE_IDX_REPORT_DIAL_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefDialIn), sizeof(hidReportRefDialIn), hidReportRefDialIn}},

        // Boot Keyboard Input Report Characteristic Declaration
        [HIDD_LE_IDX_BOOT_KB_IN_REPORT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        // Boot Keyboard Input Report Characteristic Value
        [HIDD_LE_IDX_BOOT_KB_IN_REPORT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_kb_input_uuid, ESP_GATT_PERM_READ, HIDD_LE_BOOT_REPORT_MAX_LEN, 0, NULL}},
        // Boot Keyboard Input Report Characteristic - Client Characteristic Configuration Descriptor
        [HIDD_LE_IDX_BOOT_KB_IN_REPORT_NTF_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}},

        // Boot Keyboard Output Report Characteristic Declaration
        [HIDD_LE_IDX_BOOT_KB_OUT_REPORT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
        // Boot Keyboard Output Report Characteristic Value
        [HIDD_LE_IDX_BOOT_KB_OUT_REPORT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_kb_output_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), HIDD_LE_BOOT_REPORT_MAX_LEN, 0, NULL}},

        // Boot Mouse Input Report Characteristic Declaration
        [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        // Boot Mouse Input Report Characteristic Value
        [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_mouse_input_uuid, ESP_GATT_PERM_READ, HIDD_LE_BOOT_REPORT_MAX_LEN, 0, NULL}},
        // Boot Mouse Input Report Characteristic - Client Characteristic Configuration Descriptor
        [HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}},

        // Report Characteristic Declaration
        [HIDD_LE_IDX_REPORT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},
        // Report Characteristic Value
        [HIDD_LE_IDX_REPORT_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid, ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}},
        // Report Characteristic - Report Reference Descriptor
        [HIDD_LE_IDX_REPORT_REP_REF] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid, ESP_GATT_PERM_READ, sizeof(hidReportRefFeature), sizeof(hidReportRefFeature), hidReportRefFeature}},
};

static void hid_add_id_tbl(void);

static uint16_t hid_conn_id = 0;
bool sec_conn = false;

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power*/
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00,
    /* device name */
    0x05, 0x09, 'V', 'K', 'n', 'o', 'b'};
static uint8_t raw_scan_rsp_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power */
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00};
#else
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0x12,
    0x18,
    0x00,
    0x00,
};
// 制造商数据
static uint8_t hidd_manufacturer_data[4] = "JJW";
static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,                              // 是否设置扫描响应数据，false表示不设置
    .include_name = true,                               // 是否包含设备名称，true表示包含
    .include_txpower = true,                            // 是否包含发射功率，true表示包含
    .min_interval = 0x0006,                             // 从设备连接最小间隔，时间 = min_interval * 1.25 ms
    .max_interval = 0x0010,                             // 从设备连接最大间隔，时间 = max_interval * 1.25 ms
    .appearance = 0x03c0,                               // 设备外观值，0x03c0 表示 HID 通用设备
    .manufacturer_len = sizeof(hidd_manufacturer_data), // 制造商数据长度，0 表示无制造商数据
    .p_manufacturer_data = hidd_manufacturer_data,      // 制造商数据指针，NULL 表示无数据
    .service_data_len = 0,                              // 服务数据长度，0 表示无服务数据
    .p_service_data = NULL,                             // 服务数据指针，NULL 表示无数据
    .service_uuid_len = sizeof(hidd_service_uuid128),   // 服务 UUID 长度
    .p_service_uuid = hidd_service_uuid128,             // 服务 UUID 指针，指向 HID 服务 UUID
    .flag = 0x6,                                        // 广播标志位，0x6 表示通用可发现模式
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = 0x20,                   // 广播最小间隔，单位：0.625 ms
    .adv_int_max = 0x30,                   // 广播最大间隔，单位：0.625 ms
    .adv_type = ADV_TYPE_IND,              // 广播类型，ADV_TYPE_IND 表示可连接的非定向广播
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC, // 设备地址类型，BLE_ADDR_TYPE_PUBLIC 表示公共地址
    //.peer_addr            =             // 对端地址，未使用
    // .peer_addr_type = ,             // 对端地址类型，未使用
    .channel_map = ADV_CHNL_ALL,                            // 广播通道映射，ADV_CHNL_ALL 表示使用所有通道
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // 广播过滤策略，允许任何设备扫描和连接
};
#endif
static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_attr_value_t gatts_demo_char1_val =
    {
        .attr_max_len = 0X40,
        .attr_len = sizeof(char1_str),
        .attr_value = char1_str,
};
static void gatts_DIAL_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "旋钮注册事件");
        heart_rate_profile_tab[PROFILE_APP_DIAL].service_id.is_primary = true;
        heart_rate_profile_tab[PROFILE_APP_DIAL].service_id.id.inst_id = 0x00;
        heart_rate_profile_tab[PROFILE_APP_DIAL].service_id.id.uuid.len = ESP_UUID_LEN_16;
        heart_rate_profile_tab[PROFILE_APP_DIAL].service_id.id.uuid.uuid.uuid16 = DIAL_APP_ID;

        esp_ble_gatts_create_service(gatts_if, &heart_rate_profile_tab[PROFILE_APP_DIAL].service_id, 4);
        break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "GATT_WRITE_EVT");
        break;
    case ESP_GATTS_CREATE_EVT:
        heart_rate_profile_tab[PROFILE_APP_DIAL].service_handle = param->create.service_handle;
        heart_rate_profile_tab[PROFILE_APP_DIAL].char_uuid.len = ESP_UUID_LEN_16;
        heart_rate_profile_tab[PROFILE_APP_DIAL].char_uuid.uuid.uuid16 = DIAL_APP_ID;
        esp_ble_gatts_add_char(param->create.service_handle, &heart_rate_profile_tab[PROFILE_APP_DIAL].char_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                               &gatts_demo_char1_val, NULL);
        break;
    default:
        break;
    }
}
void esp_hidd_prf_cb_hdl(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // ESP_LOGI(HID_LE_PRF_TAG, "HID_GATT event = %d gatts_if = %d", event, gatts_if);
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_HID_JOYSTICK);
        esp_hidd_cb_param_t hidd_param;
        hidd_param.init_finish.state = param->reg.status;
        if (param->reg.app_id == HIDD_APP_ID)
        {
            hidd_le_env.gatt_if = gatts_if;
            esp_ble_gap_set_device_name(HIDD_DEVICE_NAME); // 设置设备名称并配置广播数据
#ifdef CONFIG_SET_RAW_ADV_DATA
            esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
#else
            esp_ble_gap_config_adv_data(&hidd_adv_data);
#endif
            esp_ble_gatts_create_attr_tab(bas_att_db, hidd_le_env.gatt_if, BAS_IDX_NB, 0);
        }
        if (param->reg.app_id == BATTRAY_APP_ID)
        {
            hidd_param.init_finish.gatts_if = gatts_if;
            //     ESP_LOGI(HID_LE_PRF_TAG, "电池事件");
        }
        hidd_le_env.gatt_if = gatts_if;
        ESP_LOGI(HID_LE_PRF_TAG, "处理HID设备初始化完成事件.");
        break;
    case ESP_GATTS_CONF_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "创建服务完成");
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "启动服务完成");
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "gatt客户端发起连接");
        esp_hidd_cb_param_t cb_param = {0};
        memcpy(cb_param.connect.remote_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        cb_param.connect.conn_id = param->connect.conn_id;
        hidd_clcb_alloc(param->connect.conn_id, param->connect.remote_bda);
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);

        ESP_LOGI(HID_LE_PRF_TAG, "BLE连接成功");
        hid_conn_id = param->connect.conn_id;
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "gatt客户端断开连接");

        if (ble_connect_num > 0)
            ble_connect_num--;
        sec_conn = false;
        esp_ble_gap_start_advertising(&hidd_adv_params);
        uint8_t i_clcb = 0;
        hidd_clcb_t *p_clcb = NULL;
        for (i_clcb = 0, p_clcb = hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++)
        {
            memset(p_clcb, 0, sizeof(hidd_clcb_t));
        }
        break;
    case ESP_GATTS_CLOSE_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "gatt服务器关闭");
        break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "GATT READ %lu handle %d", param->read.trans_id, param->read.handle);
        // if (heart_rate_profile_tab[PROFILE_APP_BAT].UUID == BATTRAY_APP_ID)
        // {
        // esp_ble_gatts_set_attr_value(param->read.handle, 2, &battary_lev);
        // esp_ble_gatts_set_attr_value(param->read.handle, sizeof(uint8_t), &battary_lev);
        // esp_ble_gatts_send_indicate(heart_rate_profile_tab[PROFILE_APP_BAT].gatts_if, param->read.conn_id, param->read.trans_id, 2, &battary_lev, false);
        // esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &battary_lev);
        // }
        break;
    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(HID_LE_PRF_TAG, "GATT WRITE %lu handle %d", param->write.trans_id, param->read.handle);
        // esp_ble_gatts_set_attr_value(hidd_le_env.hidd_inst.att_tbl[BAS_IDX_BATT_LVL_VAL] - BAS_IDX_NB, sizeof(uint8_t), &battary_lev); // 更新数据到属性值
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:

        ESP_LOGI(HID_LE_PRF_TAG, "gatt创建表完成");
        if (param->add_attr_tab.num_handle == BAS_IDX_NB &&
            param->add_attr_tab.svc_uuid.uuid.uuid16 == ESP_GATT_UUID_BATTERY_SERVICE_SVC &&
            param->add_attr_tab.status == ESP_GATT_OK)
        {
            incl_svc.start_hdl = param->add_attr_tab.handles[BAS_IDX_SVC];
            incl_svc.end_hdl = incl_svc.start_hdl + BAS_IDX_NB - 1;
            ESP_LOGI(HID_LE_PRF_TAG, "%s(), start added the hid service to the stack database. incl_handle = %d",
                     __func__, incl_svc.start_hdl);
            esp_ble_gatts_create_attr_tab(hidd_le_gatt_db, gatts_if, HIDD_LE_IDX_NB, 0);
        }
        if (param->add_attr_tab.num_handle == HIDD_LE_IDX_NB &&
            param->add_attr_tab.status == ESP_GATT_OK)
        {
            memcpy(hidd_le_env.hidd_inst.att_tbl, param->add_attr_tab.handles,
                   HIDD_LE_IDX_NB * sizeof(uint16_t));
            ESP_LOGI(HID_LE_PRF_TAG, "hid svc handle = %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
            hid_add_id_tbl();
            esp_ble_gatts_start_service(hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
        }
        else
        {
            esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
        }
        break;
    default:
        break;
    }
}

void hidd_clcb_alloc(uint16_t conn_id, esp_bd_addr_t bda)
{
    uint8_t i_clcb = 0;
    hidd_clcb_t *p_clcb = NULL;

    for (i_clcb = 0, p_clcb = hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            p_clcb->in_use = true;
            p_clcb->conn_id = conn_id;
            p_clcb->connected = true;
            memcpy(p_clcb->remote_bda, bda, ESP_BD_ADDR_LEN);
            break;
        }
    }
    return;
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // ESP_LOGI(HID_LE_PRF_TAG, "GAT event %d, app_id %04x, gatts_if %d", (uint8_t)event, param->reg.app_id, (uint8_t)gatts_if);
    if (event == ESP_GATTS_REG_EVT && param->reg.status == ESP_GATT_OK)
    {
        if (param->reg.app_id == HIDD_APP_ID || param->reg.app_id == BATTRAY_APP_ID)
        {
            heart_rate_profile_tab[PROFILE_APP_HID].gatts_if = gatts_if;
        }
        else
            for (int idx = 0; idx < PROFILE_NUM; idx++)
            {
                if (heart_rate_profile_tab[idx].UUID == param->reg.app_id)
                {
                    ESP_LOGI(HID_LE_PRF_TAG, "注册应用 %04x 成功", heart_rate_profile_tab[idx].UUID);
                    heart_rate_profile_tab[idx].gatts_if = gatts_if;
                }
                else
                {
                    ESP_LOGI(HID_LE_PRF_TAG, "注册应用 %04x 失败", heart_rate_profile_tab[idx].UUID);
                    return;
                }
            }
    }
    for (int idx = 0; idx < PROFILE_NUM; idx++)
    {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if)
        {
            if (heart_rate_profile_tab[idx].gatts_cb)
            {
                heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

void hidd_set_attr_value(uint16_t handle, uint16_t val_len, const uint8_t *value)
{
    hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
    if (hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
        hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle)
    {
        esp_ble_gatts_set_attr_value(handle, val_len, value);
    }
    else
    {
        ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.", __func__);
    }
    return;
}

void hidd_get_attr_value(uint16_t handle, uint16_t *length, uint8_t **value)
{
    hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
    if (hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
        hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle)
    {
        esp_ble_gatts_get_attr_value(handle, length, (const uint8_t **)value);
    }
    else
    {
        ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.", __func__);
    }

    return;
}

static void hid_add_id_tbl(void)
{
    // Mouse input report
    hid_rpt_map[0].id = hidReportRefMouseIn[0];
    hid_rpt_map[0].type = hidReportRefMouseIn[1];
    hid_rpt_map[0].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_MOUSE_IN_VAL];
    hid_rpt_map[0].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_MOUSE_IN_VAL];
    hid_rpt_map[0].mode = HID_PROTOCOL_MODE_REPORT;

    // Key input report
    hid_rpt_map[1].id = hidReportRefKeyIn[0];
    hid_rpt_map[1].type = hidReportRefKeyIn[1];
    hid_rpt_map[1].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_KEY_IN_VAL];
    hid_rpt_map[1].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_KEY_IN_CCC];
    hid_rpt_map[1].mode = HID_PROTOCOL_MODE_REPORT;

    // Consumer Control input report
    hid_rpt_map[2].id = hidReportRefCCIn[0];
    hid_rpt_map[2].type = hidReportRefCCIn[1];
    hid_rpt_map[2].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_CC_IN_VAL];
    hid_rpt_map[2].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_CC_IN_CCC];
    hid_rpt_map[2].mode = HID_PROTOCOL_MODE_REPORT;

    // LED output report
    hid_rpt_map[3].id = hidReportRefLedOut[0];
    hid_rpt_map[3].type = hidReportRefLedOut[1];
    hid_rpt_map[3].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_LED_OUT_VAL];
    hid_rpt_map[3].cccdHandle = 0;
    hid_rpt_map[3].mode = HID_PROTOCOL_MODE_REPORT;

    // Boot keyboard input report
    // Use same ID and type as key input report
    hid_rpt_map[4].id = hidReportRefKeyIn[0];
    hid_rpt_map[4].type = hidReportRefKeyIn[1];
    hid_rpt_map[4].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_BOOT_KB_IN_REPORT_VAL];
    hid_rpt_map[4].cccdHandle = 0;
    hid_rpt_map[4].mode = HID_PROTOCOL_MODE_BOOT;

    // Boot keyboard output report
    // Use same ID and type as LED output report
    hid_rpt_map[5].id = hidReportRefLedOut[0];
    hid_rpt_map[5].type = hidReportRefLedOut[1];
    hid_rpt_map[5].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_BOOT_KB_OUT_REPORT_VAL];
    hid_rpt_map[5].cccdHandle = 0;
    hid_rpt_map[5].mode = HID_PROTOCOL_MODE_BOOT;

    // Boot mouse input report
    // Use same ID and type as mouse input report
    hid_rpt_map[6].id = hidReportRefMouseIn[0];
    hid_rpt_map[6].type = hidReportRefMouseIn[1];
    hid_rpt_map[6].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_BOOT_MOUSE_IN_REPORT_VAL];
    hid_rpt_map[6].cccdHandle = 0;
    hid_rpt_map[6].mode = HID_PROTOCOL_MODE_BOOT;

    // DIAL input report
    hid_rpt_map[7].id = hidReportRefDialIn[0];
    hid_rpt_map[7].type = hidReportRefDialIn[1];
    hid_rpt_map[7].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_DIAL_IN_VAL];
    hid_rpt_map[7].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_DIAL_IN_CCC];
    hid_rpt_map[7].mode = HID_PROTOCOL_MODE_REPORT;

    // Setup report ID map
    hid_dev_register_reports(HID_NUM_REPORTS, hid_rpt_map);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGE(HID_DEMO_TAG, "GAP_event %d", (uint8_t)event);
    switch (event)
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        ESP_LOGI(HID_DEMO_TAG, "开始进行BLE广播");
        // 当设置广播数据完成时，开始进行广播
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        ESP_LOGI(HID_DEMO_TAG, "开始进行BLE广播");
        // 当设置广播数据完成时，开始进行广播
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(HID_DEMO_TAG, "开始进行BLE广播");
        // 当设置广播数据完成时，开始进行广播
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(HID_DEMO_TAG, "开始进行BLE广播");
        // 当设置广播数据完成时，开始进行广播
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
#endif

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(HID_DEMO_TAG, "请求配对");
        // 打印配对请求中的设备地址，并接受配对请求
        for (int i = 0; i < ESP_BD_ADDR_LEN; i++)
        {
            ESP_LOGD(HID_DEMO_TAG, "%x:", param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        ESP_LOGI(HID_DEMO_TAG, "处理配对请求中的passkey");
        // 处理配对请求中的passkey
        break;
    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGI(HID_DEMO_TAG, "key type = %d", param->ble_security.ble_key.key_type);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        ESP_LOGI(HID_DEMO_TAG, "配对完成时");
        ble_connect_num++;
        // 配对完成时，更新安全连接状态并记录设备地址，同时打印配对结果和相关信息
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, " 地址类型 = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success)
        {
            ESP_LOGE(HID_DEMO_TAG, "失败原因 = 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_SVC %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_INCL_SVC %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_INCL_SVC]);
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_HID_INFO_CHAR %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_HID_INFO_CHAR]);
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_HID_INFO_VAL %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_HID_INFO_VAL]);
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_HID_CTNL_PT_CHAR %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_HID_CTNL_PT_CHAR]);
        // ESP_LOGI(HID_LE_PRF_TAG, "HIDD_LE_IDX_HID_CTNL_PT_VAL %d", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_HID_CTNL_PT_VAL]);

        // ESP_LOGI(HID_LE_PRF_TAG, "BAS_IDX_SVC %d", hidd_le_env.hidd_inst.att_tbl[BAS_IDX_SVC]);
        // ESP_LOGI(HID_LE_PRF_TAG, "BAS_IDX_BATT_LVL_CHAR %d", hidd_le_env.hidd_inst.att_tbl[BAS_IDX_BATT_LVL_CHAR]);
        // ESP_LOGI(HID_LE_PRF_TAG, "BAS_IDX_BATT_LVL_VAL %d", hidd_le_env.hidd_inst.att_tbl[BAS_IDX_BATT_LVL_VAL]);
        // ESP_LOGI(HID_LE_PRF_TAG, "BAS_IDX_BATT_LVL_NTF_CFG %d", hidd_le_env.hidd_inst.att_tbl[BAS_IDX_BATT_LVL_NTF_CFG]);
        // ESP_LOGI(HID_LE_PRF_TAG, "BAS_IDX_BATT_LVL_PRES_FMT %d", hidd_le_env.hidd_inst.att_tbl[BAS_IDX_BATT_LVL_PRES_FMT]);
        // if (ble_connect_num < 2)
        //     esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    default:
        break;
    }
}

/**
 * 初始化BLE HID设备
 *
 * 本函数负责初始化蓝牙低功耗（BLE）控制器和Bluedroid堆栈，
 * 并配置BLE HID设备所需的参数和回调函数
 */
esp_err_t ble_hid_init(void)
{
    // 释放经典蓝牙控制器内存
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    // 初始化蓝牙控制器
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    // 启用蓝牙控制器
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    // 初始化Bluedroid堆栈并配置默认参数
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    esp_bluedroid_init_with_cfg(&bluedroid_cfg); // 为蓝牙初始化和分配资源，必须先于每一个蓝牙的东西。
    esp_bluedroid_enable();                      // 开启蓝牙
    esp_hidd_profile_init();                     // 初始化HID设备配置文件，用于管理HID设备的通信

    // 将回调函数注册到gap模块
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(BATTRAY_APP_ID);
    esp_ble_gatts_app_register(HIDD_APP_ID);
    // esp_ble_gatts_app_register(DIAL_APP_ID);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;                // 设置认证请求模式为绑定模式，即在认证后与对端设备建立绑定关系
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;                      // 设置IO能力为无输入输出，表示设备不支持任何用户交互
    uint8_t key_size = 16;                                         // 设置密钥大小为16字节，范围为7~16字节
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK; // 设置初始化密钥类型，包括加密密钥和身份密钥
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;  // 设置响应密钥类型，同样包括加密密钥和身份密钥
    // uint32_t passkey = 123456;
    // esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t)); // 设置认证请求模式参数
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));         // 设置IO能力模式参数
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));    // 设置最大密钥大小参数
                                                                                            /* 设置初始化密钥和响应密钥：
                                                                                             * - 如果设备作为从设备（Slave），init_key表示期望主设备分发的密钥类型，rsp_key表示可以分发给主设备的密钥类型。
                                                                                             * - 如果设备作为主设备（Master），rsp_key表示期望从设备分发的密钥类型，init_key表示可以分发给从设备的密钥类型。*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    return ESP_OK;
}

esp_err_t esp_hidd_profile_init(void)
{
    if (hidd_le_env.enabled)
    {
        ESP_LOGE(HID_LE_PRF_TAG, "已初始化HID设备配置文件");
        return ESP_FAIL;
    }
    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
    return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled)
    {
        ESP_LOGE(HID_LE_PRF_TAG, "已初始化HID设备配置文件");
        return ESP_OK;
    }

    if (hidd_svc_hdl != 0)
    {
        esp_ble_gatts_stop_service(hidd_svc_hdl);
        esp_ble_gatts_delete_service(hidd_svc_hdl);
    }
    else
    {
        return ESP_FAIL;
    }

    /* register the HID device profile to the BTA_GATTS module*/
    esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);

    return ESP_OK;
}

uint16_t esp_hidd_get_version(void)
{
    return HIDD_VERSION;
}
/////////-----------------------
void ble_hid_surfacedial_report(uint8_t keycode)
{
    uint8_t report[2];
    int16_t dial_step;
    if (keycode < 2)
    {
        report[0] = keycode;
        report[1] = 0;
    }
    if (keycode == DIAL_TURN_RIGHT)
    {
        dial_step = DIAL_UNIT << 1;
        report[0] = dial_step & 0xFF;
        report[1] = dial_step >> 8;
    }
    if (keycode == DIAL_TURN_LEFT)
    {
        dial_step = -(DIAL_UNIT) << 1;
        report[0] = dial_step & 0xFF;
        report[1] = dial_step >> 8;
    }
    hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                        HID_ITF_PROTOCOL_DIAL, HID_REPORT_TYPE_INPUT, sizeof(report), report);
}
void ble_hid_mouse_report(uint8_t report_id,
                          uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal)
{
    // hid_mouse_report_t report =
    //     {
    //         .buttons = buttons,
    //         .x = x,
    //         .y = y,
    //         .wheel = vertical,
    //         .pan = horizontal};
    // hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
    //                     report_id, HID_REPORT_TYPE_INPUT, sizeof(report), &report);
}
void ble_hid_keyboard_report(uint8_t report_id, uint8_t modifier, uint8_t keycode[6])
{
    // hid_keyboard_report_t report;

    // report.modifier = modifier;
    // report.reserved = 0;

    // if (keycode)
    // {
    //     memcpy(report.keycode, keycode, sizeof(report.keycode));
    // }
    // else
    // {
    //     tu_memclr(report.keycode, 6);
    // }
    // hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
    //                     report_id, HID_REPORT_TYPE_INPUT, sizeof(report), &report);
}
void ble_hid_media_report(uint8_t report_id, uint8_t key0, uint8_t key1)
{
    uint8_t report[2];
    report[0] = key0;
    report[1] = key1;
    hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                        report_id, HID_REPORT_TYPE_INPUT, sizeof(report), report);
}
