"""Band and preamp controls for the graphic equalizer."""

import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TYPE

from .. import (
    BANDS,
    CONF_GRAPHIC_EQUALIZER_ID,
    CONF_PREAMP,
    GraphicEqualizerSpeaker,
    graphic_equalizer_ns,
    unique_control,
)

DEPENDENCIES = ["speaker"]
GraphicEqualizerNumber = graphic_equalizer_ns.class_(
    "GraphicEqualizerNumber", number.Number, cg.Component
)
CONTROLS = {**BANDS, CONF_PREAMP: 5}
CONFIG_SCHEMA = (
    number.number_schema(
        GraphicEqualizerNumber, icon="mdi:equalizer", unit_of_measurement="dB"
    )
    .extend(
        {
            cv.GenerateID(CONF_GRAPHIC_EQUALIZER_ID): cv.use_id(
                GraphicEqualizerSpeaker
            ),
            cv.Required(CONF_TYPE): cv.one_of(*CONTROLS, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
FINAL_VALIDATE_SCHEMA = unique_control("number")


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    preamp = config[CONF_TYPE] == CONF_PREAMP
    await number.register_number(
        var,
        config,
        min_value=-36 if preamp else -12,
        max_value=0 if preamp else 12,
        step=0.5,
    )
    cg.add(var.set_parent(await cg.get_variable(config[CONF_GRAPHIC_EQUALIZER_ID])))
    cg.add(var.set_control(CONTROLS[config[CONF_TYPE]]))
