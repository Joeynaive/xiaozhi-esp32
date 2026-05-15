#ifndef APDS9960_H
#define APDS9960_H

#include "i2c_device.h"
#include <cstdint>

/**
 * APDS9960 - 接近、手势、光感传感器驱动 (简化版)
 */
class Apds9960 : public I2cDevice {
public:
    Apds9960(i2c_master_bus_handle_t i2c_bus, uint8_t addr = 0x39);
    
    bool Initialize();
    
    // 读取接近值 (0-255, 越高越近)
    uint8_t ReadProximity();
    
    // 读取环境光 (Lux)
    float ReadAmbientLight();
    
    // 读取手势 (0: None, 1: Up, 2: Down, 3: Left, 4: Right)
    int ReadGesture();

private:
    void EnablePower();
    void EnableProximity();
    void EnableGesture();
    void EnableLight();
};

#endif // APDS9960_H
