#include "sensor_manager.h"
#include "apds9960.h"
#include "aht20.h"
#include "application.h"
#include "board.h"
#include "display.h"
#include "emotion_manager.h"
#include "proactive_manager.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "SensorManager"

SensorManager::SensorManager() {}
SensorManager::~SensorManager() {
    delete apds_;
    delete aht_;
}

void SensorManager::Initialize(i2c_master_bus_handle_t i2c_bus) {
    std::lock_guard<std::mutex> lock(mutex_);
    i2c_bus_ = i2c_bus;
    
    apds_ = new Apds9960(i2c_bus);
    if (!apds_->Initialize()) {
        ESP_LOGW(TAG, "APDS9960 not found or init failed");
    }
    
    aht_ = new Aht20(i2c_bus);
    if (!aht_->Initialize()) {
        ESP_LOGW(TAG, "AHT20 not found or init failed");
    }
    
    // 启动轮询任务
    xTaskCreate([](void* arg) {
        auto manager = static_cast<SensorManager*>(arg);
        while (true) {
            manager->Reload();
            vTaskDelay(pdMS_TO_TICKS(1000)); // 每 1 秒轮询一次
        }
    }, "sensor_task", 4096, this, 5, NULL);
}

void SensorManager::Reload() {
    float t = 0, h = 0, lux = 0;
    uint8_t prox = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (aht_) aht_->ReadData(t, h);
        if (apds_) {
            lux = apds_->ReadAmbientLight();
            prox = apds_->ReadProximity();
        }
        
        temperature_ = t;
        humidity_ = h;
        ambient_light_ = lux;
    }

    auto& app = Application::GetInstance();
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();

    // 1. 抚摸检测逻辑 (接近传感器触发)
    static uint32_t last_pet_time = 0;
    uint32_t now = esp_timer_get_time() / 1000000ULL;
    if (prox > 200) { 
        ProactiveManager::GetInstance().OnUserInteraction(); // 抚摸也算交互
        
        if (now - last_pet_time > 5) { // 每 5 秒最多增加一次心情
            ESP_LOGI(TAG, "Petting detected! (Proximity: %d)", prox);
            EmotionManager::GetInstance().Play(5); // 抚摸相当于陪小智玩耍，增加心情
            last_pet_time = now;
            
            // 触发 UI 快乐反馈
            display->SetEmotion("happy");
            display->InsertAnimDialog("happy", 2000); 
        }
    }
    
    // 2. 环境光自动休眠/唤醒逻辑
    static bool is_sleeping = false;
    if (lux < 5 && !is_sleeping && app.GetDeviceState() == kDeviceStateIdle) {
        is_sleeping = true;
        ESP_LOGI(TAG, "Environment is dark, entering energy saving mode");
        board.SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
        display->SetEmotion("sleepy");
    } else if (lux > 20 && is_sleeping) {
        is_sleeping = false;
        ESP_LOGI(TAG, "Environment is bright, waking up pet");
        board.SetPowerSaveLevel(PowerSaveLevel::BALANCED);
        display->SetEmotion("neutral");
    }
}

float SensorManager::GetTemperature() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return temperature_;
}

float SensorManager::GetHumidity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return humidity_;
}

float SensorManager::GetAmbientLight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ambient_light_;
}

int SensorManager::GetGesture() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_gesture_;
}
