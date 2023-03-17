import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

pt2258_ns = cg.esphome_ns.namespace("pt2258")
PT2258 = pt2258_ns.class_("PT2258", cg.Component, i2c.I2CDevice)

CONF_DEFAULT_VOLUME = "default_volume"
CONF_PT2258_ID = "pt2258_id"
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2258),
            cv.Optional(CONF_DEFAULT_VOLUME, default=0): cv.int_range(min=0, max=79),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x44))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    if CONF_DEFAULT_VOLUME in config:
        cg.add(var.set_default_volume(config[CONF_DEFAULT_VOLUME]))
