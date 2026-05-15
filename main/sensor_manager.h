#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <driver/i2c_master.h>
#include <mutex>

/**
 * SensorManager - 传感器管理中心
 * 
 * 负责初始化并定期读取环境传感器 (APDS9960, AHT20等)。
 * 为宠物提供视觉（光感/手势）和体感（温湿度）数据支持。
 */
class SensorManager {
public:
    static SensorManager& GetInstance() {
        static SensorManager instance;
        return instance;
    }

    void Initialize(i2c_master_bus_handle_t i2c_bus);
    
    // 获取传感器数值
    float GetTemperature() const;
    float GetHumidity() const;
    float GetAmbientLight() const;
    int GetGesture() const;

    // 手动触发读取 (或由内部定时器调用)
    void Reload();

private:
    SensorManager();
    ~SensorManager();

    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    class Apds9960* apds_ = nullptr;
    class Aht20* aht_ = nullptr;
    
    float temperature_ = 0.0f;
    float humidity_ = 0.0f;
    float ambient_light_ = 0.0f;
    int last_gesture_ = 0;

    mutable std::mutex mutex_;
};

#endif // SENSOR_MANAGER_H
