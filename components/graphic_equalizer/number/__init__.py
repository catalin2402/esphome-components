"""Band and preamp controls for the graphic equalizer."""

from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.types import ConfigType

from .. import (
    BANDS,
    CONF_GRAPHIC_EQUALIZER_ID,
    CONF_PREAMP,
    CURVE,
    DUMP_PUBLISHER,
    STATE_PUBLISHER,
    GraphicEqualizerSpeaker,
    unique_control,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]

CONTROLS = {**BANDS, CONF_PREAMP: 5}
CONFIG_SCHEMA = number.number_schema(
    number.ApplyNumber, icon="mdi:equalizer", unit_of_measurement="dB"
).extend(
    {
        cv.GenerateID(CONF_GRAPHIC_EQUALIZER_ID): cv.use_id(GraphicEqualizerSpeaker),
        cv.Required(CONF_TYPE): cv.one_of(*CONTROLS, lower=True),
    }
)
FINAL_VALIDATE_SCHEMA = unique_control("number")


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_GRAPHIC_EQUALIZER_ID])
    parent = parent_ref(hub)
    preamp = config[CONF_TYPE] == CONF_PREAMP
    control = CONTROLS[config[CONF_TYPE]]
    call = "set_preamp(value)" if preamp else f"set_band_gain({control}, value)"
    current = parent.get_preamp() if preamp else parent.get_band_gain(control)
    state = CURVE.preamp if preamp else CURVE.bands[control]
    numbers = number.sub_numbers(config, parent=hub)
    var = await numbers.create(
        config,
        None,
        min_value=-36 if preamp else -12,
        max_value=0 if preamp else 12,
        step=0.5,
        apply=f"{call}; const float actual = {current}; if (!entity->has_state() || entity->state != actual) entity->publish_state(actual)",
    )
    entity = parent_ref(var)
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"if (!{entity}->has_state() || {entity}->state != {state}) {entity}->publish_state({state});"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_NUMBER("", "Graphic Equalizer Gain", {entity});')
    )
