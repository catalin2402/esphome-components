from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID as CONF_ID, CONF_OFFSET, CONF_VOLUME
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
DOMAIN = "pt2258"
MULTI_CONF = True

CONF_MIN = "min"
CONF_MAX = "max"
CONF_DEFAULT_VOLUME = "default_volume"
CONF_ON_VOLUME = "on_volume"
CONF_ON_MUTE = "on_mute"
CONF_MUTE = "mute"
CONF_PT2258_ID = "pt2258_id"

pt2258_ns = cg.esphome_ns.namespace(DOMAIN)
PT2258 = pt2258_ns.class_("PT2258", cg.Component, i2c.I2CDevice)

STATE_PUBLISHER = cg.HubFunction(
    "state", cg.void, [], define="USE_PT2258_STATE_PUBLISHER"
)
DUMP_PUBLISHER = cg.HubFunction("dump", cg.void, [(cg.const_char_ptr, "TAG")])


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_VOLUME,
        "add_on_volume_callback",
        [(cg.int_, CONF_CHANNEL), (cg.int_, CONF_VOLUME)],
    ),
    automation.CallbackAutomation(
        CONF_ON_MUTE, "add_on_mute_callback", [(cg.bool_, "muted")]
    ),
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2258),
            cv.Optional(CONF_DEFAULT_VOLUME, default=0): cv.int_range(min=0, max=79),
            cv.Optional(CONF_ON_VOLUME): automation.validate_automation({}),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation({}),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x44))
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(PT2258)})


async def to_code(config: ConfigType) -> None:
    var = await i2c.new_i2c_component(config)
    cg.add(var.set_default_volume(config[CONF_DEFAULT_VOLUME]))
    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


automation.register_apply_action(
    "pt2258.resend_data",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("resend_data()"),
)

automation.register_apply_action(
    "pt2258.mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("set_mute(true)"),
)

automation.register_apply_action(
    "pt2258.unmute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("set_mute(false)"),
)

automation.register_apply_action(
    "pt2258.toggle_mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("toggle_mute()"),
)

automation.register_apply_action(
    "pt2258.set_mute",
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MUTE): cv.templatable(cv.boolean)}),
    automation.ApplyField(CONF_MUTE, "set_mute", cg.bool_),
)

automation.register_apply_action(
    "pt2258.set_master_volume",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_VOLUME): cv.templatable(cv.int_range(min=0, max=79))}
    ),
    automation.ApplyField(CONF_VOLUME, "set_master_volume", cg.int_),
)

automation.register_apply_action(
    "pt2258.set_channel_volume",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_VOLUME): cv.templatable(cv.int_range(min=0, max=79)),
        }
    ),
    automation.ApplyCall(
        "set_channel_volume({}, {})", ((CONF_VOLUME, cg.int_), (CONF_CHANNEL, cg.int_))
    ),
)

automation.register_apply_action(
    "pt2258.set_channel_offset",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_OFFSET): cv.templatable(cv.int_range(min=-79, max=79)),
        }
    ),
    automation.ApplyCall(
        "set_channel_offset({}, {})", ((CONF_OFFSET, cg.int_), (CONF_CHANNEL, cg.int_))
    ),
)

automation.register_apply_condition(
    "pt2258.is_muted", automation.maybe_simple_id(OPERATION_BASE_SCHEMA), "is_muted()"
)
automation.register_apply_condition(
    "pt2258.volume_in_range",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Optional(CONF_CHANNEL, default=0): cv.templatable(
                cv.int_range(min=0, max=6)
            ),
            cv.Optional(CONF_MIN, default=0): cv.templatable(
                cv.int_range(min=0, max=79)
            ),
            cv.Optional(CONF_MAX, default=79): cv.templatable(
                cv.int_range(min=0, max=79)
            ),
        }
    ),
    automation.ApplyCall(
        "volume_in_range({}, {}, {})",
        ((CONF_CHANNEL, cg.int_), (CONF_MIN, cg.int_), (CONF_MAX, cg.int_)),
    ),
)
