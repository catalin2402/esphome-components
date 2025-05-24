from esphome.components import sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import tuya_doorbell, CONF_TUYA2_ID, TuyaDoorbell

DEPENDENCIES = ["tuya_doorbell"]

CONF_SENSOR_DATAPOINT = "sensor_datapoint"

Tuya2Sensor = tuya_doorbell.class_("Tuya2Sensor", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = (
    sensor.sensor_schema(Tuya2Sensor)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(Tuya2Sensor),
            cv.GenerateID(CONF_TUYA2_ID): cv.use_id(TuyaDoorbell),
            cv.Required(CONF_SENSOR_DATAPOINT): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)

    paren = yield cg.get_variable(config[CONF_TUYA2_ID])
    cg.add(var.set_tuya_parent(paren))
    cg.add(var.set_sensor_id(config[CONF_SENSOR_DATAPOINT]))
