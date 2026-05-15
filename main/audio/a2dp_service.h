#ifndef A2DP_SERVICE_H
#define A2DP_SERVICE_H

#include <string>
#include <vector>
#include <mutex>
#include "sdkconfig.h"
#ifdef CONFIG_BT_ENABLED
#include "esp_a2dp_api.h"
#endif
#include "audio_codec.h"

/**
 * A2dpService - 蓝牙音箱服务
 * 
 * 负责蓝牙 A2DP Sink 模式的初始化、连接管理及音频流转发。
 */
class A2dpService {
public:
    static A2dpService& GetInstance() {
        static A2dpService instance;
        return instance;
    }

    void Initialize();
    void Start(const std::string& device_name);
    void Stop();
    
    bool IsConnected() const;
    const std::string& GetConnectedDeviceName() const;

private:
    A2dpService();
    ~A2dpService();

#ifdef CONFIG_BT_ENABLED
    // 静态回调函数，对接 ESP-IDF 的 C 接口
    static void A2dpSinkCallback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    static void A2dpSinkDataCallback(const uint8_t *data, uint32_t len);

    void HandleA2dpEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    void ProcessAudioData(const uint8_t *data, uint32_t len);
#endif

    bool is_initialized_ = false;
    bool is_running_ = false;
    bool is_connected_ = false;
    std::string connected_device_name_;
    std::string device_name_;
    
    mutable std::mutex mutex_;
};

#endif // A2DP_SERVICE_H
