import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_MOISTURE,
    CONF_TEMPERATURE,
    CONF_ILLUMINANCE,
    CONF_VERSION,
    CONF_CALIBRATION,
    CONF_OFFSET,
    CONF_ADDRESS,
    DEVICE_CLASS_MOISTURE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_ILLUMINANCE,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_CELSIUS,
    UNIT_LUX,
    ICON_RESTART,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

DEPENDENCIES = ["i2c"]

CONF_MIN_CAPACITY = "min_capacity"
CONF_MAX_CAPACITY = "max_capacity"
CONF_BRIGHTNESS_COEFFICIENT = "coefficient"
CONF_BRIGHTNESS_CONSTANT = "constant"

chirp_ns = cg.esphome_ns.namespace("chirp")
chirpComponent = chirp_ns.class_(
    "I2CSoilMoistureComponent", cg.PollingComponent, i2c.I2CDevice
)

TEMPERATURE_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Optional(CONF_CALIBRATION): cv.Schema(
            {
                cv.Optional(CONF_OFFSET): cv.int_,
            }),
        }
    )
)

MOISTURE_CONFIG_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_MOISTURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Required(CONF_CALIBRATION): cv.Schema(
            {
                cv.Optional(CONF_MIN_CAPACITY, default=290): cv.int_,
                cv.Optional(CONF_MAX_CAPACITY, default=550): cv.int_,
            }),
        }
    )
)

LUM_CONFIG_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_LUX,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_ILLUMINANCE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Optional(CONF_CALIBRATION): cv.Schema(
            {
                cv.Optional(CONF_BRIGHTNESS_COEFFICIENT, default=-1.526): cv.float_,
                cv.Optional(CONF_BRIGHTNESS_CONSTANT, default=100000): cv.int_,
            }),
        }
    )
)

VERSION_SCHEMA = (
    sensor.sensor_schema(
        icon=ICON_RESTART,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ).extend(
        {
            cv.Optional(CONF_ADDRESS): cv.templatable(cv.i2c_address),
        }
    )
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(chirpComponent),
            cv.Optional(CONF_MOISTURE): MOISTURE_CONFIG_SCHEMA,
            cv.Optional(CONF_TEMPERATURE): TEMPERATURE_SCHEMA,
            cv.Optional(CONF_ILLUMINANCE): LUM_CONFIG_SCHEMA,
            cv.Optional(CONF_VERSION): VERSION_SCHEMA,
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x20))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if moisture_config := config.get(CONF_MOISTURE):
        sens = await sensor.new_sensor(moisture_config)
        cg.add(var.set_moisture(sens))

        if calibration := moisture_config.get(CONF_CALIBRATION):
            cg.add(var.calib_capacity(
                calibration[CONF_MIN_CAPACITY],
                calibration[CONF_MAX_CAPACITY]),
            )

    if temperature_config := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature(sens))

        if calibration := temperature_config.get(CONF_CALIBRATION):
            cg.add(var.calib_temp(calibration[CONF_OFFSET]))

    if lum_config := config.get(CONF_ILLUMINANCE):
        sens = await sensor.new_sensor(lum_config)
        cg.add(var.set_light(sens))

        if calibration := lum_config.get(CONF_CALIBRATION):
            cg.add(var.calib_light(
                calibration[CONF_BRIGHTNESS_COEFFICIENT],
                calibration[CONF_BRIGHTNESS_CONSTANT]),
            )

    if version_config := config.get(CONF_VERSION):
        sens = await sensor.new_sensor(version_config)
        cg.add(var.set_version(sens))

        if address := version_config.get(CONF_ADDRESS):
            cg.add(var.new_i2c_address(address))

