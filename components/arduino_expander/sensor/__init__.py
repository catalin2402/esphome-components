from esphome.components import sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_PIN
from .. import arduino_expander_ns, CONF_ARDUINO_EXPANDER, ArduinoExpanderComponent

DEPENDENCIES = ["arduino_expander"]


ArduinoSensor = arduino_expander_ns.class_("ArduinoSensor", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    sensor.sensor_schema(ArduinoSensor)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(ArduinoSensor),
            cv.Required(CONF_ARDUINO_EXPANDER): cv.use_id(ArduinoExpanderComponent),
            cv.Required(CONF_PIN): cv.one_of(0, 1, 2, 3, 6, 7, int=True),
        }
    )
    .extend(cv.polling_component_schema("10ms"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_ARDUINO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_PIN]))
