import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
    UNIT_HOUR,
)

from .. import CONF_BAMBU_LAB_TAGS_ID, BambuLabTags, bambu_lab_tags_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["bambu_lab_tags"]

BambuLabTagsSensor = bambu_lab_tags_ns.class_(
    "BambuLabTagsSensor",
    sensor.Sensor,
    cg.Component,
    cg.Parented.template(BambuLabTags),
)
SensorType = bambu_lab_tags_ns.enum("SensorType", is_class=True)
TYPES = {
    "WEIGHT": SensorType.WEIGHT,
    "DRYING_TEMPERATURE": SensorType.DRYING_TEMPERATURE,
    "DRYING_TIME": SensorType.DRYING_TIME,
    "BED_TEMPERATURE": SensorType.BED_TEMPERATURE,
    "MIN_NOZZLE_TEMPERATURE": SensorType.MIN_NOZZLE_TEMPERATURE,
    "MAX_NOZZLE_TEMPERATURE": SensorType.MAX_NOZZLE_TEMPERATURE,
}


BASE_SCHEMA = cv.Schema(
    {cv.GenerateID(CONF_BAMBU_LAB_TAGS_ID): cv.use_id(BambuLabTags)}
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "WEIGHT": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor, unit_of_measurement="g", accuracy_decimals=0
            )
        ),
        "DRYING_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor,
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "DRYING_TIME": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor,
                accuracy_decimals=0,
                unit_of_measurement=UNIT_HOUR,
                device_class=DEVICE_CLASS_DURATION,
            )
        ),
        "BED_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor,
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "MIN_NOZZLE_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor,
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "MAX_NOZZLE_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                BambuLabTagsSensor,
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
    },
    upper=True,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_BAMBU_LAB_TAGS_ID])
    cg.add(var.set_type(TYPES[config[CONF_TYPE]]))
