from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_INPUT
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
DOMAIN = "pt2323"
MULTI_CONF = True

CONF_ENHANCE = "enhance"
CONF_BOOST = "boost"
CONF_ON_INPUT = "on_input"
CONF_ON_ENHANCE = "on_enhance"
CONF_ON_BOOST = "on_boost"
CONF_ON_MUTE = "on_mute"
CONF_MUTE = "mute"
CONF_ON_CHANNEL_MUTE = "on_channel_mute"
CONF_PT2323_ID = "pt2323_id"

pt2323_ns = cg.esphome_ns.namespace(DOMAIN)
PT2323 = pt2323_ns.class_("PT2323", cg.Component, i2c.I2CDevice)

STATE_PUBLISHER = cg.HubFunction(
    "state", cg.void, [], define="USE_PT2323_STATE_PUBLISHER"
)
DUMP_PUBLISHER = cg.HubFunction("dump", cg.void, [(cg.const_char_ptr, "TAG")])
RESTORE_PUBLISHER = cg.HubFunction("restore", cg.void, [])


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_INPUT, "add_on_input_callback", [(cg.int_, CONF_INPUT)]
    ),
    automation.CallbackAutomation(
        CONF_ON_ENHANCE, "add_on_enhance_callback", [(cg.bool_, "enabled")]
    ),
    automation.CallbackAutomation(
        CONF_ON_BOOST, "add_on_boost_callback", [(cg.bool_, "enabled")]
    ),
    automation.CallbackAutomation(
        CONF_ON_MUTE, "add_on_mute_callback", [(cg.bool_, "muted")]
    ),
    automation.CallbackAutomation(
        CONF_ON_CHANNEL_MUTE,
        "add_on_channel_mute_callback",
        [(cg.int_, CONF_CHANNEL), (cg.bool_, "muted")],
    ),
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2323),
            cv.Optional(CONF_INPUT, default=0): cv.int_range(min=0, max=4),
            cv.Optional(CONF_ENHANCE, default=False): cv.boolean,
            cv.Optional(CONF_BOOST, default=False): cv.boolean,
            cv.Optional(CONF_MUTE, default=False): cv.boolean,
            cv.Optional(CONF_ON_INPUT): automation.validate_automation({}),
            cv.Optional(CONF_ON_ENHANCE): automation.validate_automation({}),
            cv.Optional(CONF_ON_BOOST): automation.validate_automation({}),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation({}),
            cv.Optional(CONF_ON_CHANNEL_MUTE): automation.validate_automation({}),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x4A))
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(PT2323)})


async def to_code(config: ConfigType) -> None:
    var = await i2c.new_i2c_component(config)
    cg.add(var.set_input(config[CONF_INPUT]))
    cg.add(var.set_enhance(config[CONF_ENHANCE]))
    cg.add(var.set_boost(config[CONF_BOOST]))
    cg.add(var.mute_all_channels(config[CONF_MUTE]))
    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


automation.register_apply_action(
    "pt2323.resend_data",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("resend_data()"),
)

automation.register_apply_action(
    "pt2323.mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("mute_all_channels(true)"),
)

automation.register_apply_action(
    "pt2323.unmute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("mute_all_channels(false)"),
)

automation.register_apply_action(
    "pt2323.toggle_mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("mute_all_channels(!{parent}->get_mute())"),
)

automation.register_apply_action(
    "pt2323.select_input",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_INPUT): cv.templatable(cv.int_range(min=0, max=4))}
    ),
    automation.ApplyField(CONF_INPUT, "set_input", cg.int_),
)

automation.register_apply_action(
    "pt2323.set_mute",
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MUTE): cv.templatable(cv.boolean)}),
    automation.ApplyField(CONF_MUTE, "mute_all_channels", cg.bool_),
)

automation.register_apply_action(
    "pt2323.set_enhance",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_ENHANCE): cv.templatable(cv.boolean)}
    ),
    automation.ApplyField(CONF_ENHANCE, "set_enhance", cg.bool_),
)

automation.register_apply_action(
    "pt2323.set_boost",
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_BOOST): cv.templatable(cv.boolean)}),
    automation.ApplyField(CONF_BOOST, "set_boost", cg.bool_),
)

automation.register_apply_action(
    "pt2323.mute_channel",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6))}
    ),
    automation.ApplyCall("mute_channel({}, true)", ((CONF_CHANNEL, cg.int_),)),
)

automation.register_apply_action(
    "pt2323.unmute_channel",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6))}
    ),
    automation.ApplyCall("mute_channel({}, false)", ((CONF_CHANNEL, cg.int_),)),
)

automation.register_apply_action(
    "pt2323.set_channel_mute",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_MUTE): cv.templatable(cv.boolean),
        }
    ),
    automation.ApplyCall(
        "mute_channel({}, {})", ((CONF_CHANNEL, cg.int_), (CONF_MUTE, cg.bool_))
    ),
)

automation.register_apply_condition(
    "pt2323.is_muted", automation.maybe_simple_id(OPERATION_BASE_SCHEMA), "get_mute()"
)

automation.register_apply_condition(
    "pt2323.is_enhanced",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "get_enhance()",
)

automation.register_apply_condition(
    "pt2323.is_boosted",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "get_boost()",
)

automation.register_apply_condition(
    "pt2323.selected_input",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_INPUT): cv.templatable(cv.int_range(min=0, max=4))}
    ),
    automation.ApplyCall("get_input() == {}", ((CONF_INPUT, cg.int_),)),
)
automation.register_apply_condition(
    "pt2323.is_channel_muted",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6))}
    ),
    automation.ApplyCall("is_channel_muted({})", ((CONF_CHANNEL, cg.int_),)),
)
