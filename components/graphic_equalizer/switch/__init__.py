"""Enable or bypass the graphic equalizer."""

import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv

from .. import (
    CONF_GRAPHIC_EQUALIZER_ID,
    GraphicEqualizerSpeaker,
    graphic_equalizer_ns,
    unique_control,
)

DEPENDENCIES = ["speaker"]
GraphicEqualizerSwitch = graphic_equalizer_ns.class_(
    "GraphicEqualizerSwitch", switch.Switch, cg.Component
)
CONFIG_SCHEMA = (
    switch.switch_schema(
        GraphicEqualizerSwitch,
        icon="mdi:equalizer",
        default_restore_mode="DISABLED",
        block_inverted=True,
    )
    .extend(
        {
            cv.GenerateID(CONF_GRAPHIC_EQUALIZER_ID): cv.use_id(
                GraphicEqualizerSpeaker
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
FINAL_VALIDATE_SCHEMA = unique_control("switch")


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_GRAPHIC_EQUALIZER_ID])))
