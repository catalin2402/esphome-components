"""Select entity for sound-reactive lighting modes."""

import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_ID
import esphome.final_validate as fv

from .. import (
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    SoundReactiveLightsSpeaker,
    sound_reactive_lights_ns,
)

DEPENDENCIES = ["speaker"]
SoundReactiveLightsSelect = sound_reactive_lights_ns.class_(
    "SoundReactiveLightsSelect", select.Select, cg.Component
)
OPTIONS = ["Off", "VU", "Spectrum", "Rainbow Pulse"]
CONFIG_SCHEMA = (
    select.select_schema(SoundReactiveLightsSelect)
    .extend(
        {
            cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
                SoundReactiveLightsSpeaker
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def final_validate(config):
    matching = [
        item
        for item in fv.full_config.get().get("select", [])
        if item.get("platform") == "sound_reactive_lights"
        and item[CONF_SOUND_REACTIVE_LIGHTS_ID] == config[CONF_SOUND_REACTIVE_LIGHTS_ID]
    ]
    if len(matching) > 1:
        raise cv.Invalid(
            "Only one mode select may control each sound_reactive_lights speaker"
        )

    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    cg.add_define("USE_SOUND_REACTIVE_MODE_SELECT")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=OPTIONS)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])))
