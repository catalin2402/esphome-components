import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_FREQUENCY
from esphome import automation

DEPENDENCIES = ["i2c"]

tea5767_ns = cg.esphome_ns.namespace("tea5767")
TEA5767 = tea5767_ns.class_("TEA5767", cg.Component, i2c.I2CDevice)
SetFrequencyAutomation = tea5767_ns.class_("SetFrequencyAction", automation.Action)

CONF_DEFAULT_VOLUME = "default_volume"
CONF_IN_EUROPE = "in_europe"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TEA5767),
            cv.Optional(CONF_IN_EUROPE, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x60))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.setInEurope(config[CONF_IN_EUROPE]))

OPERATION_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(TEA5767),
    }
)


@automation.register_action(
    "tea5767.set_frequency",
    SetFrequencyAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_FREQUENCY): cv.frequency,
            }
        )
    ),
)
async def tea5767_set_frequency_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_FREQUENCY], args, cg.uint16)
    cg.add(var.set_frequency(template_))
    return var    