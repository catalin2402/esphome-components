import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INPUT, CONF_OPTIONS
import esphome.final_validate as fv

from .. import CONF_PT2323_ID, PT2323, pt2323_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["pt2323"]


PT2323Select = pt2323_ns.class_("PT2323Select", select.Select, cg.Component)

CONFIG_SCHEMA = (
    select.select_schema(PT2323Select)
    .extend(
        {
            cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
            cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string_strict), cv.Length(min=1, max=5)
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=config[CONF_OPTIONS])
    parent = await cg.get_variable(config[CONF_PT2323_ID])
    cg.add(var.set_parent(parent))


def _final_validate(config):
    if len(set(config[CONF_OPTIONS])) != len(config[CONF_OPTIONS]):
        raise cv.Invalid("options must have unique names")
    full_config = fv.full_config.get()
    path = full_config.get_path_for_id(config[CONF_PT2323_ID])[:-1]
    parent = full_config.get_config_for_path(path)
    if parent[CONF_INPUT] >= len(config[CONF_OPTIONS]):
        raise cv.Invalid("options must include the hub's configured input index")
    return config


FINAL_VALIDATE_SCHEMA = _final_validate
