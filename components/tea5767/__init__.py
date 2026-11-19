from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_FREQUENCY
from esphome.types import ConfigType

from ._publisher import Publisher

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
DOMAIN = "tea5767"
MULTI_CONF = True

CONF_ON_FREQUENCY = "on_frequency"
CONF_ON_MUTE = "on_mute"
CONF_ON_MONO = "on_mono"
CONF_ON_SOFT_MUTE = "on_soft_mute"
CONF_ON_HIGH_CUT_CONTROL = "on_high_cut_control"
CONF_ON_STEREO_NOISE_CANCELING = "on_stereo_noise_canceling"
CONF_ON_STEREO = "on_stereo"
CONF_ON_LEVEL = "on_level"
CONF_MIN = "min"
CONF_MAX = "max"

tea5767_ns = cg.esphome_ns.namespace(DOMAIN)
TEA5767 = tea5767_ns.class_("TEA5767", cg.PollingComponent, i2c.I2CDevice)
CONF_TEA5767_ID = "tea5767_id"
CONF_IN_JAPAN = "in_japan"
CONF_MUTE = "mute"
CONF_MONO = "mono"
CONF_SOFT_MUTE = "soft_mute"
CONF_STEREO_NOISE_CANCELING = "stereo_noise_canceling"
CONF_HIGH_CUT_CONTROL = "high_cut_control"
Setting = tea5767_ns.enum("Setting", is_class=True)

STATE_PUBLISHER = Publisher("state", cg.void, [], define="USE_TEA5767_STATE_PUBLISHER")
DUMP_PUBLISHER = Publisher("dump", cg.void, [(cg.const_char_ptr, "TAG")])
RESTORE_PUBLISHER = Publisher("restore", cg.void, [])
STATUS_PUBLISHER = Publisher(
    "status",
    cg.void,
    [(cg.uint64, "frequency"), (cg.uint8, "level"), (cg.bool_, "stereo")],
    define="USE_TEA5767_STATUS_PUBLISHER",
)
FREQUENCY = STATUS_PUBLISHER.param("frequency")
LEVEL = STATUS_PUBLISHER.param("level")
STEREO = STATUS_PUBLISHER.param("stereo")


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_FREQUENCY, "add_on_frequency_callback", [(cg.uint64, "frequency")]
    ),
    automation.CallbackAutomation(
        CONF_ON_MUTE,
        "add_on_setting_callback",
        [(cg.bool_, "enabled")],
        params=[(Setting, "setting"), (cg.bool_, "enabled")],
        forward=[cg.RawExpression("enabled")],
        when=f"setting == {Setting.SETTING_MUTE}",
    ),
    automation.CallbackAutomation(
        CONF_ON_MONO,
        "add_on_setting_callback",
        [(cg.bool_, "enabled")],
        params=[(Setting, "setting"), (cg.bool_, "enabled")],
        forward=[cg.RawExpression("enabled")],
        when=f"setting == {Setting.SETTING_MONO}",
    ),
    automation.CallbackAutomation(
        CONF_ON_SOFT_MUTE,
        "add_on_setting_callback",
        [(cg.bool_, "enabled")],
        params=[(Setting, "setting"), (cg.bool_, "enabled")],
        forward=[cg.RawExpression("enabled")],
        when=f"setting == {Setting.SETTING_SOFT_MUTE}",
    ),
    automation.CallbackAutomation(
        CONF_ON_STEREO_NOISE_CANCELING,
        "add_on_setting_callback",
        [(cg.bool_, "enabled")],
        params=[(Setting, "setting"), (cg.bool_, "enabled")],
        forward=[cg.RawExpression("enabled")],
        when=f"setting == {Setting.SETTING_STEREO_NOISE_CANCELING}",
    ),
    automation.CallbackAutomation(
        CONF_ON_HIGH_CUT_CONTROL,
        "add_on_setting_callback",
        [(cg.bool_, "enabled")],
        params=[(Setting, "setting"), (cg.bool_, "enabled")],
        forward=[cg.RawExpression("enabled")],
        when=f"setting == {Setting.SETTING_HIGH_CUT_CONTROL}",
    ),
    automation.CallbackAutomation(
        CONF_ON_STEREO, "add_on_stereo_callback", [(cg.bool_, "stereo")]
    ),
    automation.CallbackAutomation(
        CONF_ON_LEVEL, "add_on_level_callback", [(cg.uint8, "level")]
    ),
)

FREQUENCY_SCHEMA = cv.All(cv.frequency, cv.Range(min=76e6, max=108e6))


def validate_config(config: ConfigType) -> ConfigType:
    if not config[CONF_IN_JAPAN] and config[CONF_FREQUENCY] < 87.5e6:
        raise cv.Invalid(
            "Frequencies below 87.5 MHz require in_japan: true", path=[CONF_FREQUENCY]
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TEA5767),
            cv.Optional(CONF_FREQUENCY, default="87.5MHz"): FREQUENCY_SCHEMA,
            cv.Optional(CONF_IN_JAPAN, default=False): cv.boolean,
            cv.Optional(CONF_MUTE, default=False): cv.boolean,
            cv.Optional(CONF_MONO, default=False): cv.boolean,
            cv.Optional(CONF_SOFT_MUTE, default=False): cv.boolean,
            cv.Optional(CONF_STEREO_NOISE_CANCELING, default=False): cv.boolean,
            cv.Optional(CONF_HIGH_CUT_CONTROL, default=False): cv.boolean,
            cv.Optional(CONF_ON_FREQUENCY): automation.validate_automation({}),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation({}),
            cv.Optional(CONF_ON_MONO): automation.validate_automation({}),
            cv.Optional(CONF_ON_SOFT_MUTE): automation.validate_automation({}),
            cv.Optional(CONF_ON_STEREO_NOISE_CANCELING): automation.validate_automation(
                {}
            ),
            cv.Optional(CONF_ON_HIGH_CUT_CONTROL): automation.validate_automation({}),
            cv.Optional(CONF_ON_STEREO): automation.validate_automation({}),
            cv.Optional(CONF_ON_LEVEL): automation.validate_automation({}),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x60)),
    validate_config,
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(TEA5767)})


async def to_code(config: ConfigType) -> None:
    var = await i2c.new_i2c_component(config)
    cg.add(var.set_in_japan(config[CONF_IN_JAPAN]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_mute(config[CONF_MUTE]))
    cg.add(var.set_mono(config[CONF_MONO]))
    cg.add(var.set_soft_mute(config[CONF_SOFT_MUTE]))
    cg.add(var.set_stereo_noise_canceling(config[CONF_STEREO_NOISE_CANCELING]))
    cg.add(var.set_high_cut_control(config[CONF_HIGH_CUT_CONTROL]))
    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


automation.register_apply_action(
    "tea5767.set_frequency",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_FREQUENCY): cv.templatable(FREQUENCY_SCHEMA)}
    ),
    automation.ApplyField(CONF_FREQUENCY, "set_frequency", cg.uint64),
)

automation.register_apply_action(
    "tea5767.set_mute",
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MUTE): cv.templatable(cv.boolean)}),
    automation.ApplyField(CONF_MUTE, "set_mute", cg.bool_),
)

automation.register_apply_action(
    "tea5767.set_mono",
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MONO): cv.templatable(cv.boolean)}),
    automation.ApplyField(CONF_MONO, "set_mono", cg.bool_),
)

automation.register_apply_action(
    "tea5767.set_soft_mute",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_SOFT_MUTE): cv.templatable(cv.boolean)}
    ),
    automation.ApplyField(CONF_SOFT_MUTE, "set_soft_mute", cg.bool_),
)

automation.register_apply_action(
    "tea5767.set_stereo_noise_canceling",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_STEREO_NOISE_CANCELING): cv.templatable(cv.boolean)}
    ),
    automation.ApplyField(
        CONF_STEREO_NOISE_CANCELING, "set_stereo_noise_canceling", cg.bool_
    ),
)

automation.register_apply_action(
    "tea5767.set_high_cut_control",
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_HIGH_CUT_CONTROL): cv.templatable(cv.boolean)}
    ),
    automation.ApplyField(CONF_HIGH_CUT_CONTROL, "set_high_cut_control", cg.bool_),
)

automation.register_apply_action(
    "tea5767.mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("set_mute(true)"),
)

automation.register_apply_action(
    "tea5767.unmute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("set_mute(false)"),
)

automation.register_apply_action(
    "tea5767.toggle_mute",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("toggle_mute()"),
)

automation.register_apply_action(
    "tea5767.resend_data",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("resend_data()"),
)

automation.register_apply_condition(
    "tea5767.is_muted",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_muted()",
)

automation.register_apply_condition(
    "tea5767.is_mono",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_mono()",
)

automation.register_apply_condition(
    "tea5767.is_soft_muted",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_soft_muted()",
)

automation.register_apply_condition(
    "tea5767.stereo_noise_canceling_enabled",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "get_stereo_noise_canceling()",
)

automation.register_apply_condition(
    "tea5767.high_cut_control_enabled",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "get_high_cut_control()",
)

automation.register_apply_condition(
    "tea5767.is_stereo",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_stereo()",
)

automation.register_apply_condition(
    "tea5767.frequency_in_range",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Optional(CONF_MIN, default="76MHz"): cv.templatable(FREQUENCY_SCHEMA),
            cv.Optional(CONF_MAX, default="108MHz"): cv.templatable(FREQUENCY_SCHEMA),
        }
    ),
    automation.ApplyCall(
        "frequency_in_range({}, {})", ((CONF_MIN, cg.uint64), (CONF_MAX, cg.uint64))
    ),
)

automation.register_apply_condition(
    "tea5767.signal_level_in_range",
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Optional(CONF_MIN, default=0): cv.templatable(
                cv.int_range(min=0, max=15)
            ),
            cv.Optional(CONF_MAX, default=15): cv.templatable(
                cv.int_range(min=0, max=15)
            ),
        }
    ),
    automation.ApplyCall(
        "signal_level_in_range({}, {})", ((CONF_MIN, cg.int_), (CONF_MAX, cg.int_))
    ),
)
