from esphome import automation
import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.types import ConfigType

from .. import (
    CONF_PT2323_ID,
    DUMP_PUBLISHER,
    PT2323,
    RESTORE_PUBLISHER,
    STATE_PUBLISHER,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["pt2323"]

CONF_CHANNEL_A = "channel_a"
CONF_CHANNEL_B = "channel_b"


def validate_channels(value):
    if value[CONF_TYPE] != "MUTE" and (
        CONF_CHANNEL_A in value or CONF_CHANNEL_B in value
    ):
        raise cv.Invalid("channel_a and channel_b are only supported for type: mute")
    if (
        value[CONF_TYPE] == "MUTE"
        and CONF_CHANNEL_A not in value
        and CONF_CHANNEL_B not in value
    ):
        raise cv.Invalid("At least one channel is needed for mute switch")
    return value


CONFIG_SCHEMA = cv.All(
    switch.switch_schema(switch.ApplySwitch, default_restore_mode="DISABLED").extend(
        {
            cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
            cv.Required(CONF_TYPE): cv.one_of(
                "ENHANCE", "BOOST", "MUTE", "MUTE_ALL", upper=True
            ),
            cv.Optional(CONF_CHANNEL_A): cv.int_range(min=1, max=6),
            cv.Optional(CONF_CHANNEL_B): cv.int_range(min=1, max=6),
        }
    ),
    validate_channels,
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PT2323_ID])
    channel_a = config.get(CONF_CHANNEL_A, 0)
    channel_b = config.get(CONF_CHANNEL_B, 0)
    parent = automation.parent_ref(hub)
    kind = config[CONF_TYPE]
    if kind == "MUTE":
        calls = [
            f"mute_channel({channel}, state)"
            for channel in (channel_a, channel_b)
            if channel
        ]
        apply = f"; {parent}->".join(calls)
        state = parent.get_channel_mute(channel_a or channel_b)
    else:
        method, getter = {
            "ENHANCE": ("set_enhance", "get_enhance"),
            "BOOST": ("set_boost", "get_boost"),
            "MUTE_ALL": ("mute_all_channels", "get_mute"),
        }[kind]
        apply = f"{method}(state)"
        state = getattr(parent, getter)()
    switches = switch.sub_switches(config, parent=hub)
    var = await switches.create(config, None, apply=apply)
    entity = automation.parent_ref(var)
    RESTORE_PUBLISHER.add(hub, entity.restore())
    STATE_PUBLISHER.add(hub, entity.publish_state(state))
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SWITCH("", "PT2323 Switch", {entity});')
    )
    DUMP_PUBLISHER.add(hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Type: {kind}");'))
    DUMP_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f'ESP_LOGCONFIG(TAG, "  Channel A: {channel_a}, Channel B: {channel_b}");'
        ),
    )
