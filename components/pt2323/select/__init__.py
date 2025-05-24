from esphome.components import select
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_OPTIONS, CONF_ID
from .. import pt2323_ns, CONF_PT2323_ID, PT2323

DEPENDENCIES = ["pt2323"]


PT2323Select = pt2323_ns.class_("PT2323Select", select.Select, cg.Component)

CONFIG_SCHEMA = (
    select.select_schema(PT2323Select)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(PT2323Select),
            cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
            cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string_strict), cv.Length(min=1, max=5)
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=config[CONF_OPTIONS])
    parent = await cg.get_variable(config[CONF_PT2323_ID])
    cg.add(var.set_parent(parent))
