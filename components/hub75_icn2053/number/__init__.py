from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY_ID
from esphome.types import ConfigType

from .. import BRIGHTNESS, DUMP_PUBLISHER, STATE_PUBLISHER, HUB75Display

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["hub75_icn2053"]

CONFIG_SCHEMA = number.number_schema(
    number.ApplyNumber, icon="mdi:brightness-6"
).extend({cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(HUB75Display)})


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_DISPLAY_ID])
    numbers = number.sub_numbers(config, parent=hub)
    var = await numbers.create(
        config,
        None,
        min_value=0,
        max_value=255,
        step=1,
        apply="control_brightness(entity, value)",
    )
    entity = parent_ref(var)
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"if (!{entity}->has_state() || {entity}->state != {BRIGHTNESS}) {entity}->publish_state({BRIGHTNESS});"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_NUMBER("", "HUB75 Brightness", {entity});')
    )
