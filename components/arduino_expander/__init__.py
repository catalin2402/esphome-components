import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_NUMBER,
    CONF_MODE,
    CONF_INVERTED,
    CONF_OUTPUT,
    CONF_PULLUP,
)

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

arduino_expander_ns = cg.esphome_ns.namespace("arduino_expander")
ArduinoExpanderComponent = arduino_expander_ns.class_(
    "ArduinoExpander", cg.Component, i2c.I2CDevice
)
ArduinoGPIOPin = arduino_expander_ns.class_("ArduinoGPIOPin", cg.GPIOPin)

AdcSource = arduino_expander_ns.enum("AdcSource")

CONF_ARDUINO_EXPANDER = "arduino_expander"
CONF_ADC_SOURCE = "adc_source"

ADC_SOURCES = {
    "DEFAULT": AdcSource.DEFAULT,
    "INTERNAL": AdcSource.INTERNAL,
    "EXTERNAL": AdcSource.EXTERNAL,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(ArduinoExpanderComponent),
            cv.Optional(CONF_ADC_SOURCE): cv.enum(ADC_SOURCES, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x10))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    if CONF_ADC_SOURCE in config:
        cg.add(var.set_adc_source(config[CONF_ADC_SOURCE]))


def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_PULLUP] and not value[CONF_INPUT]:
        raise cv.Invalid("Pullup only available with input")
    return value


ARDUINO_EXPANDER_PIN_SCHEMA = cv.All(
    {
        cv.GenerateID(): cv.declare_id(ArduinoGPIOPin),
        cv.Required(CONF_ARDUINO_EXPANDER): cv.use_id(ArduinoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=13),
        cv.Optional(CONF_MODE, default={}): cv.All(
            {
                cv.Optional(CONF_INPUT, default=False): cv.boolean,
                cv.Optional(CONF_PULLUP, default=False): cv.boolean,
                cv.Optional(CONF_OUTPUT, default=False): cv.boolean,
            },
            validate_mode,
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)


@pins.PIN_SCHEMA_REGISTRY.register("arduino_expander", ARDUINO_EXPANDER_PIN_SCHEMA)
async def arduino_expander_pins_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_ARDUINO_EXPANDER])
    cg.add(var.set_parent(parent))
    num = config[CONF_NUMBER]
    cg.add(var.set_pin(num))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
