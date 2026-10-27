"""Sound Reactive Lights switch."""

import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
import esphome.final_validate as fv

from .. import (
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    SoundReactiveLightsSpeaker,
    sound_reactive_lights_ns,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]

SoundReactiveLightsSwitch = sound_reactive_lights_ns.class_(
    "SoundReactiveLightsSwitch", switch.Switch, cg.Component
)
CONFIG_SCHEMA = (
    switch.switch_schema(
        SoundReactiveLightsSwitch,
        icon="mdi:music-note",
        default_restore_mode="DISABLED",
        block_inverted=True,
    )
    .extend(
        {
            cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
                SoundReactiveLightsSpeaker
            )
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def final_validate(config):
    matching = [
        item
        for item in fv.full_config.get().get("switch", [])
        if item.get("platform") == "sound_reactive_lights"
        and item[CONF_SOUND_REACTIVE_LIGHTS_ID] == config[CONF_SOUND_REACTIVE_LIGHTS_ID]
    ]
    if len(matching) > 1:
        raise cv.Invalid(
            "Only one sound reactive lights switch may control each parent"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])))
