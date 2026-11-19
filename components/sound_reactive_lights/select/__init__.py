"""Select entity for sound-reactive lighting modes."""

from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.types import ConfigType

from .. import (
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    DOMAIN,
    DUMP_PUBLISHER,
    MODE,
    STATE_PUBLISHER,
    SoundReactiveLightsSpeaker,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]

OPTIONS = ["Off", "VU", "Spectrum", "Rainbow Pulse"]
CONFIG_SCHEMA = select.select_schema(select.ApplySelect).extend(
    {
        cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
            SoundReactiveLightsSpeaker
        ),
    }
)


FINAL_VALIDATE_SCHEMA = fv.unique_platform_key(
    domain="select",
    platform=DOMAIN,
    keys=(CONF_SOUND_REACTIVE_LIGHTS_ID,),
    message="Only one mode select may control each sound_reactive_lights speaker",
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])
    parent = parent_ref(hub)
    selects = select.sub_selects(config, parent=hub)
    var = await selects.create(
        config,
        None,
        options=OPTIONS,
        apply=f"set_mode(index); {parent}->set_enabled(true)",
    )
    entity = parent_ref(var)
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"if ({entity}->active_index() != {MODE}) {entity}->publish_state(static_cast<size_t>({MODE}));"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SELECT("", "Sound Reactive Lights Mode", {entity});')
    )
