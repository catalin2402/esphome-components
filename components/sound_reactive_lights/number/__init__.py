"""Runtime controls for reactive light sensitivity and brightness."""

import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TYPE
import esphome.final_validate as fv

from .. import (
    CONF_MAX_BRIGHTNESS,
    CONF_SENSITIVITY,
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    SoundReactiveLightsSpeaker,
    sound_reactive_lights_ns,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]

SoundReactiveLightsNumber = sound_reactive_lights_ns.class_(
    "SoundReactiveLightsNumber", number.Number, cg.Component
)
NumberType = sound_reactive_lights_ns.enum("ReactiveNumberType")
TYPES = {
    CONF_SENSITIVITY: NumberType.NUMBER_SENSITIVITY,
    CONF_MAX_BRIGHTNESS: NumberType.NUMBER_MAX_BRIGHTNESS,
}
PARENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
            SoundReactiveLightsSpeaker
        ),
    }
).extend(cv.COMPONENT_SCHEMA)
CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_SENSITIVITY: number.number_schema(
            SoundReactiveLightsNumber, icon="mdi:tune"
        ).extend(PARENT_SCHEMA),
        CONF_MAX_BRIGHTNESS: number.number_schema(
            SoundReactiveLightsNumber, icon="mdi:brightness-6", unit_of_measurement="%"
        ).extend(PARENT_SCHEMA),
    },
    lower=True,
)


def final_validate(config):
    matching = [
        item
        for item in fv.full_config.get().get("number", [])
        if item.get("platform") == "sound_reactive_lights"
        and item[CONF_SOUND_REACTIVE_LIGHTS_ID] == config[CONF_SOUND_REACTIVE_LIGHTS_ID]
        and item[CONF_TYPE] == config[CONF_TYPE]
    ]
    if len(matching) > 1:
        raise cv.Invalid(
            "Only one number of each type may control each reactive speaker"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    sensitivity = config[CONF_TYPE] == CONF_SENSITIVITY
    await number.register_number(
        var,
        config,
        min_value=0.1 if sensitivity else 0,
        max_value=100,
        step=0.1 if sensitivity else 1,
    )
    cg.add(var.set_parent(await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])))
    cg.add(var.set_type(TYPES[config[CONF_TYPE]]))
