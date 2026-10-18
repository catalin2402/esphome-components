import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_OUTPUT_ID

from .. import STATUS_LIGHT_SCHEMA, StatusLight, setup_status_light, status_light_ns

AUTO_LOAD = ["status_light"]

StatusLightLightOutput = status_light_ns.class_(
    "StatusLightLightOutput", light.LightOutput, StatusLight
)

CONFIG_SCHEMA = (
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(StatusLightLightOutput),
        }
    )
    .extend(STATUS_LIGHT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await setup_status_light(var, config)
    await cg.register_component(var, config)
    await light.register_light(var, config)
