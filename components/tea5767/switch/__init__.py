from esphome import automation
import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.types import ConfigType

from .. import (
    CONF_TEA5767_ID,
    DUMP_PUBLISHER,
    RESTORE_PUBLISHER,
    STATE_PUBLISHER,
    TEA5767,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tea5767"]

CONFIG_SCHEMA = cv.All(
    switch.switch_schema(switch.ApplySwitch, default_restore_mode="DISABLED").extend(
        {
            cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
            cv.Required(CONF_TYPE): cv.one_of(
                "MUTE",
                "MONO",
                "SOFT_MUTE",
                "STEREO_NOISE_CANCELING",
                "HIGH_CUT_CONTROL",
                upper=True,
            ),
        }
    )
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_TEA5767_ID])
    parent = automation.parent_ref(hub)
    kind = config[CONF_TYPE]
    setting = kind.lower()
    switches = switch.sub_switches(config, parent=hub)
    var = await switches.create(config, None, apply=f"set_{setting}(state)")
    entity = automation.parent_ref(var)
    if kind == "MUTE":
        state = parent.is_muted()
    elif kind == "MONO":
        state = parent.is_mono()
    elif kind == "SOFT_MUTE":
        state = parent.is_soft_muted()
    elif kind == "STEREO_NOISE_CANCELING":
        state = parent.get_stereo_noise_canceling()
    else:
        state = parent.get_high_cut_control()
    RESTORE_PUBLISHER.add(hub, entity.restore())
    STATE_PUBLISHER.add(hub, entity.publish_state(state))
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SWITCH("", "TEA5767 Switch", {entity});')
    )
    DUMP_PUBLISHER.add(hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Type: {kind}");'))
