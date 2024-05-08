#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace chirp {

class I2CSoilMoistureComponent : public PollingComponent, public i2c::I2CDevice, public sensor::Sensor {
 public:
  void set_moisture(sensor::Sensor *moisture) { moisture_ = moisture; }
  void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
  void set_light(sensor::Sensor *light) { light_ = light; }
  void set_address(uint8_t addr) { device_.new_addr = addr; }
  void calib_temp(float tOff) { calibration_.t_offset = tOff; }
  void calib_capacity(int16_t cMin, int16_t cMax) {
    calibration_.c_Min = cMin;
    calibration_.c_Max = cMax;
  }
  void calib_light(float coeficient, int32_t constant) {
    calibration_.l_coeficient = coeficient;
    calibration_.l_constant = constant;
  }

  void update() override;
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  struct CalibrationData {
    int16_t c_Min = 290;          // Capacity when wet.
    int16_t c_Max = 550;          // Capacity when dry.
    float l_coeficient = -1.525;  // Sensor specific coefficient.
    int32_t l_constant = 100000;  // Direct sunlight.
    float t_offset = 0;           // Temperature offset.
  };

  struct Device {
    uint32_t interval = 0;
    bool started = false;
    uint8_t addr = 0;
    uint8_t new_addr = 0;
  };

  // Internal method to read the moisture from the component after it has been scheduled.
  bool read_moisture_();
  // Internal method to read the temperature from the component after it has been scheduled.
  bool read_temperature_();
  // Internal method to read the light from the component after it has been scheduled.
  bool read_light_();
  // Internal method to initialize the light measurement with a 3 second read delay.
  bool measure_light();
  // Internal method to read the firmware version of the sensor.
  uint8_t read_version_();
  // Internal method to read the I2C address of the sensor.
  uint8_t read_address_();
  // Internal method to write the I2C address of the sensor.
  bool write_address(uint8_t new_addr);
  // Internal method to read the busy status from the sensor.
  bool read_busy_();
  // Internal method to reset the sensor.
  bool write_reset_();

  sensor::Sensor *moisture_{nullptr};
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *light_{nullptr};

  CalibrationData calibration_;
  Device device_;
};

}  // namespace chirp
}  // namespace esphome
