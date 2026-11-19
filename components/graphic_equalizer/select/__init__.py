"""Preset selector for the graphic equalizer."""

from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.types import ConfigType

from .. import (
    CONF_GRAPHIC_EQUALIZER_ID,
    DUMP_PUBLISHER,
    PRESET,
    STATE_PUBLISHER,
    GraphicEqualizerSpeaker,
    unique_control,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]

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
CONFIG_SCHEMA = select.select_schema(select.ApplySelect, icon="mdi:equalizer").extend(
    {
        cv.GenerateID(CONF_GRAPHIC_EQUALIZER_ID): cv.use_id(GraphicEqualizerSpeaker),
    }
)
FINAL_VALIDATE_SCHEMA = unique_control("select")


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_GRAPHIC_EQUALIZER_ID])
    selects = select.sub_selects(config, parent=hub)
    var = await selects.create(
        config, None, options=OPTIONS, apply="select_preset(index)"
    )
    entity = parent_ref(var)
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"if ({entity}->active_index() != {PRESET}) {entity}->publish_state(static_cast<size_t>({PRESET}));"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SELECT("", "Graphic Equalizer Preset", {entity});')
    )
