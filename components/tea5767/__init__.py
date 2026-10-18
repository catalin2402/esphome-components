from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_FREQUENCY, CONF_ID, CONF_TRIGGER_ID

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
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

tea5767_ns = cg.esphome_ns.namespace("tea5767")
TEA5767 = tea5767_ns.class_("TEA5767", cg.PollingComponent, i2c.I2CDevice)
CONF_TEA5767_ID = "tea5767_id"
CONF_IN_JAPAN = "in_japan"
CONF_MUTE = "mute"
CONF_MONO = "mono"
CONF_SOFT_MUTE = "soft_mute"
CONF_STEREO_NOISE_CANCELING = "stereo_noise_canceling"
CONF_HIGH_CUT_CONTROL = "high_cut_control"
SetFrequencyAction = tea5767_ns.class_("SetFrequencyAction", automation.Action)
ToggleMuteAction = tea5767_ns.class_("ToggleMuteAction", automation.Action)
ResendDataAction = tea5767_ns.class_("ResendDataAction", automation.Action)
SetMuteAction = tea5767_ns.class_("SetMuteAction", automation.Action)
SetMonoAction = tea5767_ns.class_("SetMonoAction", automation.Action)
SetSoftMuteAction = tea5767_ns.class_("SetSoftMuteAction", automation.Action)
SetStereoNoiseCancelingAction = tea5767_ns.class_(
    "SetStereoNoiseCancelingAction", automation.Action
)
SetHighCutControlAction = tea5767_ns.class_(
    "SetHighCutControlAction", automation.Action
)
IsMutedCondition = tea5767_ns.class_("IsMutedCondition", automation.Condition)
IsMonoCondition = tea5767_ns.class_("IsMonoCondition", automation.Condition)
IsSoftMutedCondition = tea5767_ns.class_("IsSoftMutedCondition", automation.Condition)
IsStereoNoiseCancelingCondition = tea5767_ns.class_(
    "IsStereoNoiseCancelingCondition", automation.Condition
)
IsHighCutControlCondition = tea5767_ns.class_(
    "IsHighCutControlCondition", automation.Condition
)
IsStereoCondition = tea5767_ns.class_("IsStereoCondition", automation.Condition)
FrequencyInRangeCondition = tea5767_ns.class_(
    "FrequencyInRangeCondition", automation.Condition
)
SignalLevelInRangeCondition = tea5767_ns.class_(
    "SignalLevelInRangeCondition", automation.Condition
)
FrequencyTrigger = tea5767_ns.class_(
    "FrequencyTrigger", automation.Trigger.template(cg.uint64)
)
MuteTrigger = tea5767_ns.class_("MuteTrigger", automation.Trigger.template(cg.bool_))
MonoTrigger = tea5767_ns.class_("MonoTrigger", automation.Trigger.template(cg.bool_))
SoftMuteTrigger = tea5767_ns.class_(
    "SoftMuteTrigger", automation.Trigger.template(cg.bool_)
)
StereoNoiseCancelingTrigger = tea5767_ns.class_(
    "StereoNoiseCancelingTrigger", automation.Trigger.template(cg.bool_)
)
HighCutControlTrigger = tea5767_ns.class_(
    "HighCutControlTrigger", automation.Trigger.template(cg.bool_)
)
StereoTrigger = tea5767_ns.class_(
    "StereoTrigger", automation.Trigger.template(cg.bool_)
)
LevelTrigger = tea5767_ns.class_("LevelTrigger", automation.Trigger.template(cg.uint8))

FREQUENCY_SCHEMA = cv.All(cv.frequency, cv.Range(min=76e6, max=108e6))


def validate_config(config):
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
            cv.Optional(CONF_ON_FREQUENCY): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FrequencyTrigger)}
            ),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MuteTrigger)}
            ),
            cv.Optional(CONF_ON_MONO): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MonoTrigger)}
            ),
            cv.Optional(CONF_ON_SOFT_MUTE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SoftMuteTrigger)}
            ),
            cv.Optional(CONF_ON_STEREO_NOISE_CANCELING): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        StereoNoiseCancelingTrigger
                    )
                }
            ),
            cv.Optional(CONF_ON_HIGH_CUT_CONTROL): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(HighCutControlTrigger)}
            ),
            cv.Optional(CONF_ON_STEREO): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StereoTrigger)}
            ),
            cv.Optional(CONF_ON_LEVEL): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LevelTrigger)}
            ),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x60)),
    validate_config,
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(TEA5767)})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_in_japan(config[CONF_IN_JAPAN]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_mute(config[CONF_MUTE]))
    cg.add(var.set_mono(config[CONF_MONO]))
    cg.add(var.set_soft_mute(config[CONF_SOFT_MUTE]))
    cg.add(var.set_stereo_noise_canceling(config[CONF_STEREO_NOISE_CANCELING]))
    cg.add(var.set_high_cut_control(config[CONF_HIGH_CUT_CONTROL]))
    for conf in config.get(CONF_ON_FREQUENCY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint64, "frequency")], conf)
    for conf in config.get(CONF_ON_MUTE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_MONO, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_SOFT_MUTE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_STEREO_NOISE_CANCELING, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_HIGH_CUT_CONTROL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_STEREO, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "stereo")], conf)
    for conf in config.get(CONF_ON_LEVEL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint8, "level")], conf)


@automation.register_action(
    "tea5767.set_frequency",
    SetFrequencyAction,
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_FREQUENCY): cv.templatable(FREQUENCY_SCHEMA)}
    ),
    synchronous=True,
)
async def set_frequency_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_FREQUENCY], args, cg.uint64)
    cg.add(var.set_frequency(value))
    return var


@automation.register_action(
    "tea5767.set_mute",
    SetMuteAction,
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MUTE): cv.templatable(cv.boolean)}),
    synchronous=True,
)
async def set_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_MUTE], args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "tea5767.set_mono",
    SetMonoAction,
    OPERATION_BASE_SCHEMA.extend({cv.Required(CONF_MONO): cv.templatable(cv.boolean)}),
    synchronous=True,
)
async def set_mono_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_MONO], args, cg.bool_)
    cg.add(var.set_mono(value))
    return var


@automation.register_action(
    "tea5767.set_soft_mute",
    SetSoftMuteAction,
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_SOFT_MUTE): cv.templatable(cv.boolean)}
    ),
    synchronous=True,
)
async def set_soft_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_SOFT_MUTE], args, cg.bool_)
    cg.add(var.set_soft_mute(value))
    return var


@automation.register_action(
    "tea5767.set_stereo_noise_canceling",
    SetStereoNoiseCancelingAction,
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_STEREO_NOISE_CANCELING): cv.templatable(cv.boolean)}
    ),
    synchronous=True,
)
async def set_stereo_noise_canceling_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_STEREO_NOISE_CANCELING], args, cg.bool_)
    cg.add(var.set_stereo_noise_canceling(value))
    return var


@automation.register_action(
    "tea5767.set_high_cut_control",
    SetHighCutControlAction,
    OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_HIGH_CUT_CONTROL): cv.templatable(cv.boolean)}
    ),
    synchronous=True,
)
async def set_high_cut_control_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_HIGH_CUT_CONTROL], args, cg.bool_)
    cg.add(var.set_high_cut_control(value))
    return var


@automation.register_action(
    "tea5767.mute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mute(True))
    return var


@automation.register_action(
    "tea5767.unmute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def unmute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mute(False))
    return var


@automation.register_action(
    "tea5767.toggle_mute",
    ToggleMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def toggle_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "tea5767.resend_data",
    ResendDataAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def resend_data_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_condition(
    "tea5767.is_muted",
    IsMutedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_muted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.is_mono",
    IsMonoCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_mono_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.is_soft_muted",
    IsSoftMutedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_soft_muted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.stereo_noise_canceling_enabled",
    IsStereoNoiseCancelingCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def stereo_noise_canceling_enabled_to_code(
    config, condition_id, template_arg, args
):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.high_cut_control_enabled",
    IsHighCutControlCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def high_cut_control_enabled_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.is_stereo",
    IsStereoCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_stereo_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "tea5767.frequency_in_range",
    FrequencyInRangeCondition,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Optional(CONF_MIN, default="76MHz"): cv.templatable(FREQUENCY_SCHEMA),
            cv.Optional(CONF_MAX, default="108MHz"): cv.templatable(FREQUENCY_SCHEMA),
        }
    ),
)
async def frequency_in_range_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    for key in (CONF_MIN, CONF_MAX):
        value = await cg.templatable(config[key], args, cg.uint64)
        cg.add(getattr(var, f"set_{key}")(value))
    return var


@automation.register_condition(
    "tea5767.signal_level_in_range",
    SignalLevelInRangeCondition,
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
)
async def signal_level_in_range_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    for key in (CONF_MIN, CONF_MAX):
        value = await cg.templatable(config[key], args, cg.int_)
        cg.add(getattr(var, f"set_{key}")(value))
    return var
