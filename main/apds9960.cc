#include "apds9960.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "APDS9960"

Apds9960::Apds9960(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr) {}

bool Apds9960::Initialize() {
    uint8_t id = ReadReg(0x92);
    if (id != 0xAB && id != 0x92) { // 某些克隆版 ID 可能不同
        ESP_LOGE(TAG, "Device ID mismatch: 0x%02X", id);
        return false;
    }
    
    EnablePower();
    vTaskDelay(pdMS_TO_TICKS(10));
    EnableProximity();
    EnableLight();
    
    ESP_LOGI(TAG, "APDS9960 initialized");
    return true;
}

void Apds9960::EnablePower() {
    WriteReg(0x80, 0x01); // PON
}

void Apds9960::EnableProximity() {
    WriteReg(0x80, ReadReg(0x80) | 0x04 | 0x20); // PEN + PIEN
}

void Apds9960::EnableLight() {
    WriteReg(0x80, ReadReg(0x80) | 0x02); // AEN
}

uint8_t Apds9960::ReadProximity() {
    return ReadReg(0x9C);
}

float Apds9960::ReadAmbientLight() {
    uint8_t low = ReadReg(0x94);
    uint8_t high = ReadReg(0x95);
    return (float)(high << 8 | low);
}

int Apds9960::ReadGesture() {
    // 简化的手势读取逻辑，实际手势识别需要复杂的 FIFO 处理
    // 这里仅作为占位
    return 0;
}
