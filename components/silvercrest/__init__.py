import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID


silvercrest_ns = cg.esphome_ns.namespace("silvercrest")
Silvercrest = silvercrest_ns.class_("Silvercrest", cg.Component)

CONF_TRANSMITTER_PIN = "transmitter_pin"
CONF_SILVERCREST_ID = "silvercrest"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Silvercrest),
        cv.Required(CONF_TRANSMITTER_PIN): pins.gpio_output_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    transmitter_pin = await cg.gpio_pin_expression(config[CONF_TRANSMITTER_PIN])
    cg.add(var.set_transmitter_pin(transmitter_pin))
