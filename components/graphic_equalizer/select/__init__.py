"""Preset selector for the graphic equalizer."""

import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import (
    CONF_GRAPHIC_EQUALIZER_ID,
    GraphicEqualizerSpeaker,
    graphic_equalizer_ns,
    unique_control,
)

DEPENDENCIES = ["speaker"]
GraphicEqualizerSelect = graphic_equalizer_ns.class_(
    "GraphicEqualizerSelect", select.Select, cg.Component
)
OPTIONS = [
    "Flat",
    "Bass Boost",
    "Treble Boost",
    "Vocal",
    "Rock",
    "User 1",
    "User 2",
    "User 3",
]
CONFIG_SCHEMA = (
    select.select_schema(GraphicEqualizerSelect, icon="mdi:equalizer")
    .extend(
        {
            cv.GenerateID(CONF_GRAPHIC_EQUALIZER_ID): cv.use_id(
                GraphicEqualizerSpeaker
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
FINAL_VALIDATE_SCHEMA = unique_control("select")


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=OPTIONS)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_GRAPHIC_EQUALIZER_ID])))
