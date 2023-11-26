import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, binary_sensor, sensor
from esphome.const import CONF_ID, CONF_FREQUENCY
from esphome import automation

DEPENDENCIES = ["i2c"]

AUTO_LOAD = ["binary_sensor", "sensor"]

tea5767_ns = cg.esphome_ns.namespace("tea5767")
TEA5767 = tea5767_ns.class_("TEA5767", cg.PollingComponent, i2c.I2CDevice)
SetFrequencyAutomation = tea5767_ns.class_("SetFrequencyAction", automation.Action)
SetMuteAutomation = tea5767_ns.class_("SetMuteAction", automation.Action)
SetMonoAutomation = tea5767_ns.class_("SetMonoAction", automation.Action)
SetSoftMuteAutomation = tea5767_ns.class_("SetSoftMuteAutomation", automation.Action)
SetStereoNoiseCancelingAutomation = tea5767_ns.class_(
    "SetStereoNoiseCancelingAutomation", automation.Action
)
SetHighCutControlAction = tea5767_ns.class_(
    "SetHighCutControlAction", automation.Action
)

CONF_TEA5767_ID = "tea5767_id"
CONF_DEFAULT_VOLUME = "default_volume"
CONF_IN_JAPAN = "in_japan"
CONF_MUTE = "mute"
CONF_MONO = "mono"
CONF_MONO_SENSOR = "mono_sensor"
CONF_LEVEL_SENSOR = "level_sensor"
CONF_FREQUENCY_SENSOR = "frequency_sensor"
CONF_SOFT_MUTE = "soft_mute"
CONF_STEREO_NOISE_CANCELING = "stereo_noise_canceling"
CONF_HIGH_CUT_CONTROL = "high_cut_control"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TEA5767),
            cv.Optional(CONF_FREQUENCY): cv.All(
                cv.frequency, cv.Range(min=875e5, max=108e6)
            ),
            cv.Optional(CONF_IN_JAPAN, default=False): cv.boolean,
            cv.Optional(CONF_MONO_SENSOR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_LEVEL_SENSOR): sensor.sensor_schema(),
            cv.Optional(CONF_FREQUENCY_SENSOR): sensor.sensor_schema(),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x60))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_in_japan(config[CONF_IN_JAPAN]))

    if CONF_FREQUENCY in config:
        cg.add(var.set_frequency(config[CONF_FREQUENCY]))

    if CONF_MONO_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_MONO_SENSOR])
        cg.add(var.set_mono_sensor(sens))

    if CONF_LEVEL_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_LEVEL_SENSOR])
        cg.add(var.set_level_sensor(sens))

    if CONF_FREQUENCY_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_FREQUENCY_SENSOR])
        cg.add(var.set_frequency_sensor(sens))


OPERATION_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(TEA5767),
    }
)


@automation.register_action(
    "tea5767.set_frequency",
    SetFrequencyAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_FREQUENCY): cv.All(
                    cv.frequency, cv.Range(min=875e5, max=108e6)
                ),
            }
        )
    ),
)
async def tea5767_set_frequency_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_FREQUENCY], args, cg.uint64)
    cg.add(var.set_frequency(template_))
    return var


@automation.register_action(
    "tea5767.set_mute",
    SetMuteAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_MUTE): cv.boolean,
            }
        )
    ),
)
async def tea5767_set_mute_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_MUTE], args, cg.bool_)
    cg.add(var.set_mute(template_))
    return var


@automation.register_action(
    "tea5767.set_mono",
    SetMonoAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_MONO): cv.boolean,
            }
        )
    ),
)
async def tea5767_set_mono_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_MONO], args, cg.bool_)
    cg.add(var.set_mono(template_))
    return var


@automation.register_action(
    "tea5767.set_soft_mute",
    SetSoftMuteAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_SOFT_MUTE): cv.boolean,
            }
        )
    ),
)
async def tea5767_set_soft_mute_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_SOFT_MUTE], args, cg.bool_)
    cg.add(var.set_soft_mute(template_))
    return var


@automation.register_action(
    "tea5767.set_stereo_noise_canceling",
    SetStereoNoiseCancelingAutomation,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_STEREO_NOISE_CANCELING): cv.boolean,
            }
        )
    ),
)
async def tea5767_set_stereo_noise_canceling_to_code(
    config, action_id, template_arg, args
):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(
        config[CONF_STEREO_NOISE_CANCELING], args, cg.bool_
    )
    cg.add(var.set_stereo_noise_canceling(template_))
    return var


@automation.register_action(
    "tea5767.set_high_cut_control",
    SetHighCutControlAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_HIGH_CUT_CONTROL): cv.boolean,
            }
        )
    ),
)
async def tea5767_set_mono_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_HIGH_CUT_CONTROL], args, cg.bool_)
    cg.add(var.set_high_cut_control(template_))
    return var
