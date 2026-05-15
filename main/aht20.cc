#include "aht20.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "AHT20"

Aht20::Aht20(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr) {}

bool Aht20::Initialize() {
    if (i2c_device_ == nullptr) return false;
    vTaskDelay(pdMS_TO_TICKS(40));
    // 检查状态，确保已校准
    uint8_t status = ReadReg(0x71);
    if (!(status & 0x08)) {
        uint8_t init_cmd[] = {0xBE, 0x08, 0x00};
        i2c_master_transmit(i2c_device_, init_cmd, 3, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "AHT20 initialized");
    return true;
}

void Aht20::TriggerMeasurement() {
    if (i2c_device_ == nullptr) return;
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_master_transmit(i2c_device_, cmd, 3, 100);
}

bool Aht20::ReadData(float& temperature, float& humidity) {
    if (i2c_device_ == nullptr) return false;
    TriggerMeasurement();
    vTaskDelay(pdMS_TO_TICKS(80));
    
    uint8_t buffer[7];
    // AHT20 读取不需要写寄存器地址，直接读取
    i2c_master_receive(i2c_device_, buffer, 7, 100);
    
    if (buffer[0] & 0x80) { // Busy
        return false;
    }
    
    uint32_t hum_raw = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | (buffer[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
    
    humidity = (float)hum_raw * 100.0f / 1048576.0f;
    temperature = (float)temp_raw * 200.0f / 1048576.0f - 50.0f;
    
    return true;
}
