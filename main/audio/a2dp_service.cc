#include "a2dp_service.h"
#include "application.h"
#include "board.h"
#include <esp_log.h>
#include "sdkconfig.h"

#ifdef CONFIG_BT_ENABLED
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_a2dp_api.h>
#endif
#include <cstring>

#define TAG "A2dpService"

A2dpService::A2dpService() {}
A2dpService::~A2dpService() {
    Stop();
}

void A2dpService::Initialize() {
#ifdef CONFIG_BT_ENABLED
    if (is_initialized_) return;

    esp_err_t ret;
    // 1. 释放低功耗蓝牙内存 (如果只需要经典蓝牙)
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // 2. 初始化蓝牙控制器
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // 3. 初始化 Bluedroid 堆栈
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    is_initialized_ = true;
    ESP_LOGI(TAG, "A2DP Service initialized");
#else
    ESP_LOGW(TAG, "Bluetooth is not enabled in sdkconfig. A2DP Service will not start.");
#endif
}

void A2dpService::Start(const std::string& device_name) {
#ifdef CONFIG_BT_ENABLED
    if (!is_initialized_) Initialize();
    if (is_running_) return;

    device_name_ = device_name;
    esp_bt_dev_set_device_name(device_name_.c_str());

    // 初始化 A2DP Sink
    esp_a2d_register_callback(&A2dpService::A2dpSinkCallback);
    esp_a2d_sink_register_data_callback(&A2dpService::A2dpSinkDataCallback);
    esp_a2d_sink_init();

    // 设置可发现和可连接模式
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    is_running_ = true;
    ESP_LOGI(TAG, "A2DP Service started with name: %s", device_name_.c_str());
#endif
}

void A2dpService::Stop() {
#ifdef CONFIG_BT_ENABLED
    if (!is_running_) return;

    esp_a2d_sink_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    is_running_ = false;
    is_connected_ = false;
    is_initialized_ = false;
#endif
}

#ifdef CONFIG_BT_ENABLED
void A2dpService::A2dpSinkCallback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    GetInstance().HandleA2dpEvent(event, param);
}

void A2dpService::A2dpSinkDataCallback(const uint8_t *data, uint32_t len) {
    GetInstance().ProcessAudioData(data, len);
}

void A2dpService::HandleA2dpEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT:
            if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI(TAG, "A2DP connected");
                is_connected_ = true;
                // 切换应用状态到蓝牙模式
                Application::GetInstance().SetDeviceState(kDeviceStateBluetoothMode);
            } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "A2DP disconnected");
                is_connected_ = false;
                Application::GetInstance().SetDeviceState(kDeviceStateIdle);
            }
            break;
        case ESP_A2D_AUDIO_STATE_EVT:
            if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
                ESP_LOGI(TAG, "A2DP audio started");
            } else if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STOPPED) {
                ESP_LOGI(TAG, "A2DP audio stopped");
            }
            break;
        default:
            break;
    }
}

void A2dpService::ProcessAudioData(const uint8_t *data, uint32_t len) {
    // 将蓝牙音频数据直接写入 Codec
    // 注意：A2DP 通常是 44.1kHz Stereo，我们需要确保 Codec 配置匹配或进行重采样
    // 这里的 data 是 16bit PCM 格式
    auto codec = Board::GetInstance().GetAudioCodec();
    
    // 临时简单处理：直接写入。如果采样率不匹配会有变音，后续可优化
    std::vector<int16_t> pcm(len / 2);
    std::memcpy(pcm.data(), data, len);
    codec->OutputData(pcm);
}
#endif

bool A2dpService::IsConnected() const {
    return is_connected_;
}
