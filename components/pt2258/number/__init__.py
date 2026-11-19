from esphome import automation
import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_TYPE
from esphome.types import ConfigType

from .. import CONF_PT2258_ID, DUMP_PUBLISHER, PT2258, STATE_PUBLISHER

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["pt2258"]

CONF_CHANNEL_A = "channel_a"
CONF_CHANNEL_B = "channel_b"


def validate_channels(value):
    if value[CONF_MIN_VALUE] >= value[CONF_MAX_VALUE]:
        raise cv.Invalid("min_value must be less than max_value")
    if value[CONF_TYPE] != "OFFSET" and value[CONF_MIN_VALUE] < 0:
        raise cv.Invalid("Negative values are only supported for type: offset")
    if (
        value[CONF_TYPE] != "MASTER"
        and CONF_CHANNEL_A not in value
        and CONF_CHANNEL_B not in value
    ):
        raise cv.Invalid("At least one channel is needed")
    if value[CONF_TYPE] == "MASTER" and (
        CONF_CHANNEL_A in value or CONF_CHANNEL_B in value
    ):
        raise cv.Invalid("Please remove channel_a and channel_b from config")
    return value


CONFIG_SCHEMA = cv.All(
    number.number_schema(number.ApplyNumber).extend(
        {
            cv.GenerateID(CONF_PT2258_ID): cv.use_id(PT2258),
            cv.Optional(CONF_TYPE, default="master"): cv.one_of(
                "MASTER", "VOLUME", "OFFSET", upper=True
            ),
            cv.Optional(CONF_CHANNEL_A): cv.int_range(min=1, max=6),
            cv.Optional(CONF_CHANNEL_B): cv.int_range(min=1, max=6),
            cv.Optional(CONF_MIN_VALUE, default=0): cv.int_range(min=-79, max=79),
            cv.Optional(CONF_MAX_VALUE, default=79): cv.int_range(min=-79, max=79),
        }
    ),
    validate_channels,
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PT2258_ID])
    channel_a = config.get(CONF_CHANNEL_A, 0)
    channel_b = config.get(CONF_CHANNEL_B, 0)
    offset = config[CONF_TYPE] == "OFFSET"
    numbers = number.sub_numbers(config, parent=hub)
    var = await numbers.create(
        config,
        None,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=1,
        apply=f"control_number(entity, value, {channel_a}, {channel_b}, {str(offset).lower()})",
    )
    parent = automation.parent_ref(hub)
    entity = automation.parent_ref(var)
    channel = channel_a or channel_b
    state = parent.get_channel_volume(channel)
    if offset:
        state = parent.get_channel_volume(channel, True) - parent.get_channel_volume(
            0, True
        )
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"{{ const float value = {state}; if (!{entity}->has_state() || {entity}->state != value) {entity}->publish_state(value); }}"
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_NUMBER("", "PT2258 Number", {entity});')
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Type: {config[CONF_TYPE]}");')
    )
    DUMP_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f'ESP_LOGCONFIG(TAG, "  Channel A: {channel_a}, Channel B: {channel_b}");'
        ),
    )
