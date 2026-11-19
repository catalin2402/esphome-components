"""Runtime controls for reactive light sensitivity and brightness."""

from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
import esphome.final_validate as fv
from esphome.types import ConfigType

from .. import (
    CONF_MAX_BRIGHTNESS,
    CONF_SENSITIVITY,
    CONF_SOUND_REACTIVE_LIGHTS_ID,
    DOMAIN,
    DUMP_PUBLISHER,
    MAX_BRIGHTNESS,
    SENSITIVITY,
    STATE_PUBLISHER,
    SoundReactiveLightsSpeaker,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["speaker"]


PARENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SOUND_REACTIVE_LIGHTS_ID): cv.use_id(
            SoundReactiveLightsSpeaker
        ),
    }
)
CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_SENSITIVITY: number.number_schema(
            number.ApplyNumber, icon="mdi:tune"
        ).extend(PARENT_SCHEMA),
        CONF_MAX_BRIGHTNESS: number.number_schema(
            number.ApplyNumber, icon="mdi:brightness-6", unit_of_measurement="%"
        ).extend(PARENT_SCHEMA),
    },
    lower=True,
)


FINAL_VALIDATE_SCHEMA = fv.unique_platform_key(
    domain="number",
    platform=DOMAIN,
    keys=(CONF_SOUND_REACTIVE_LIGHTS_ID, CONF_TYPE),
    message="Only one number of each type may control each reactive speaker",
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SOUND_REACTIVE_LIGHTS_ID])
    parent = parent_ref(hub)
    sensitivity = config[CONF_TYPE] == CONF_SENSITIVITY
    call = (
        "set_sensitivity(value)"
        if sensitivity
        else "set_max_brightness(value / 100.0f)"
    )
    current = (
        parent.get_sensitivity() if sensitivity else parent.get_max_brightness() * 100.0
    )
    state = SENSITIVITY if sensitivity else MAX_BRIGHTNESS * 100.0
    numbers = number.sub_numbers(config, parent=hub)
    var = await numbers.create(
        config,
        None,
        min_value=0.1 if sensitivity else 0,
        max_value=100,
        step=0.1 if sensitivity else 1,
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
        hub, cg.RawStatement(f'LOG_NUMBER("", "Sound Reactive Lights", {entity});')
    )
