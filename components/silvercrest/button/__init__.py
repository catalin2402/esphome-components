from esphome import automation
import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_BUTTON
from esphome.types import ConfigType

from .. import CHANNELS, CONF_SILVERCREST_ID, DUMP_PUBLISHER, Silvercrest

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["silvercrest", "remote_transmitter"]

BUTTON_TYPE = {
    "A_ON": 0,
    "B_ON": 1,
    "C_ON": 2,
    "D_ON": 3,
    "M_ON": 4,
    "A_OFF": 5,
    "B_OFF": 6,
    "C_OFF": 7,
    "D_OFF": 8,
    "M_OFF": 9,
}


CONFIG_SCHEMA = button.button_schema(button.ApplyButton).extend(
    cv.Schema(
        {
            cv.GenerateID(CONF_SILVERCREST_ID): cv.use_id(Silvercrest),
            cv.Required(CONF_BUTTON): cv.enum(BUTTON_TYPE, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SILVERCREST_ID])
    button_type = BUTTON_TYPE[config[CONF_BUTTON]]
    channel = CHANNELS[("A", "B", "C", "D", "MASTER")[button_type % 5]]
    state = str(button_type < 5).lower()
    buttons = button.sub_buttons(config, parent=hub)
    var = await buttons.create(
        config, None, apply=f"control_channel({channel}, {state})"
    )
    entity = automation.parent_ref(var)
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_BUTTON("", "Silvercrest Button", {entity});')
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Button: {button_type}");')
    )
