"""Sound Reactive Lights switch."""

from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.types import ConfigType

from .. import (
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    DOMAIN,
    DUMP_PUBLISHER,
    ENABLED,
    RESTORE_PUBLISHER,
    STATE_PUBLISHER,
    SoundReactiveLightsSpeaker,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]


CONFIG_SCHEMA = switch.switch_schema(
    switch.ApplySwitch,
    icon="mdi:music-note",
    default_restore_mode="DISABLED",
    block_inverted=True,
).extend(
    {
        cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
            SoundReactiveLightsSpeaker
        )
    }
)


FINAL_VALIDATE_SCHEMA = fv.unique_platform_key(
    domain="switch",
    platform=DOMAIN,
    keys=(CONF_SOUND_REACTIVE_LIGHTS_ID,),
    message="Only one sound reactive lights switch may control each parent",
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])
    parent = parent_ref(hub)
    switches = switch.sub_switches(config, parent=hub)
    var = await switches.create(
        config,
        None,
        apply=f"set_enabled(state); entity->publish_state({parent}->is_enabled())",
    )
    entity = parent_ref(var)
    RESTORE_PUBLISHER.add(hub, entity.restore())
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"if (!{entity}->has_state() || {entity}->state != {ENABLED}) {entity}->publish_state({ENABLED});"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SWITCH("", "Sound Reactive Lights", {entity});')
    )
