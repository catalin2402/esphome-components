import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_TYPE
from .. import A2DPSink, CONF_A2DP_SINK_ID, ns

DEPENDENCIES = ["a2dp_sink"]
BluetoothSwitch = ns.class_("BluetoothSwitch", switch.Switch, cg.Component)
CONFIG_SCHEMA = switch.switch_schema(BluetoothSwitch).extend({
    cv.GenerateID(CONF_A2DP_SINK_ID): cv.use_id(A2DPSink),
    cv.Optional(CONF_TYPE, default="bluetooth"): cv.one_of("bluetooth", lower=True),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_A2DP_SINK_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_enabled(False))
