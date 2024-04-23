#include "chirp.h"
#include "esphome/core/log.h"

namespace esphome {
namespace chirp {

static const char *const TAG = "I2CSensor";

enum RegisterAddress : uint8_t {
  GET_CAPACITANCE = 0x00,  // (r)  2 bytes
  SET_ADDRESS = 0x01,      // (w)  1 byte
  GET_ADDRESS = 0x02,      // (r)  1 byte
  MEASURE_LIGHT = 0x03,    // (w)  n/a
  GET_LIGHT = 0x04,        // (r)  2 bytes
  GET_TEMPERATURE = 0x05,  // (r)  2 bytes
  RESET = 0x06,            // (w)  n/a
  GET_VERSION = 0x07,      // (r)  1 bytes
  SLEEP = 0x08,            // (w)  n/a
  GET_BUSY = 0x09          // (r)  1 bytes
};

void I2CSoilMoistureComponent::update() {
  if (read_busy_()) {
    status_set_warning("Sensor is busy.");
    return;
  }

  status_clear_warning();

  if (moisture_ != nullptr) {
    if (!read_moisture_()) {
      status_set_error("Failed to read moisture.");
      return;
    }
  }

  if (temperature_ != nullptr) {
    if (!read_temperature_()) {
      status_set_error("Failed to read temperature.");
      return;
    }
  }

  if (light_ != nullptr) {
    if (!read_light_()) {
      status_set_error("Failed to read light.");
      return;
    }
  }

  status_clear_error();
}

void I2CSoilMoistureComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sensor...");

  if (!read_version_()) {
    status_set_error("Failed to read version.");
    mark_failed();
    return;
  }

  update();

  if (status_has_error()) {
    mark_failed();
    return;
  }

  started_ = true;
}

void I2CSoilMoistureComponent::dump_config() {
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);

  if (is_failed()) {
    ESP_LOGE(TAG, "Connection failed!");
  }

  ESP_LOGCONFIG(TAG, "  Firmware Version: %.1f", read_version_() / 100.0f);

  LOG_SENSOR("  ", "Moisture", moisture_);
  LOG_SENSOR("  ", "Temperature", temperature_);
  LOG_SENSOR("  ", "Light", light_);
}

void I2CSoilMoistureComponent::new_i2c_address(uint8_t addr) {
  if (read_address_() == addr) {
    ESP_LOGCONFIG(TAG, "Address already set to 0x%02X", addr);
    status_clear_error();
    return;
  }

  if (!write_byte(SET_ADDRESS, addr)) {
    status_set_error("Failed to write address.");
    return;
  }

  if (!write_byte(RESET, 1)) {
    status_set_error("Failed to reset.");
    return;
  }

  set_i2c_address(addr);
  status_clear_error();

  ESP_LOGW(TAG, "Changed I2C address to 0x%02X", addr);
}

bool I2CSoilMoistureComponent::read_moisture_() {
  uint8_t buffer[2];
  if (!read_bytes(GET_CAPACITANCE, buffer, 2)) {
    return false;
  }

  int16_t raw = (buffer[0] << 8) | buffer[1];

  // Ensure the raw value is within bounds.
  if (raw < calibration_.c_Min)
    raw = calibration_.c_Min;
  else if (raw > calibration_.c_Max)
    raw = calibration_.c_Max;

  // Calculate moisture percentage.
  float moisture = ((raw - calibration_.c_Min) * 100) / (calibration_.c_Max - calibration_.c_Min);

  if (started_) {
    moisture_->publish_state(moisture);
  }

  return true;
}

bool I2CSoilMoistureComponent::read_temperature_() {
  uint8_t buffer[2];
  if (!read_bytes(GET_TEMPERATURE, buffer, 2)) {
    return false;
  }

  int16_t raw = (buffer[0] << 8) | buffer[1];
  float temp = raw / 256.0f;

  if (started_) {
    temperature_->publish_state(temp + calibration_.t_offset);
  }

  return true;
}

bool I2CSoilMoistureComponent::read_light_() {
  if (!write_byte(MEASURE_LIGHT, 1)) {
    return false;
  }

  set_timeout("light", 3, [this]() {
    uint8_t buffer[2];
    if (!read_bytes(GET_LIGHT, buffer, 2)) {
      status_set_error("GET_LIGHT: Read failed");
      return;
    }

    int16_t raw = (buffer[0] << 8) | buffer[1];

    // Apply the conversion formula to get light in lux
    float light = calibration_.l_coeficient * raw + calibration_.l_constant;

    if (started_) {
      light_->publish_state(light);
    }
  });

  return true;
}

bool I2CSoilMoistureComponent::read_version_() {
  uint8_t buffer[1];

  if (!read_bytes(GET_VERSION, buffer, 1)) {
    return false;
  }

  if (this->version_ != nullptr) {
    this->version_->publish_state(buffer[0] / 100.0f);
  }

  return true;
}

bool I2CSoilMoistureComponent::read_busy_() {
  uint8_t buffer[1];
  if (!read_bytes(GET_BUSY, buffer, 1)) {
    return false;
  }

  return buffer[0] == 1;
}

uint8_t I2CSoilMoistureComponent::read_address_() {
  uint8_t buffer[1];
  if (!read_bytes(GET_ADDRESS, buffer, 1)) {
    return 0;
  }

  return buffer[0];
}

}  // namespace chirp
}  // namespace esphome
