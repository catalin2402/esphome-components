from esphome import automation
import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_INPUT, CONF_OPTIONS
import esphome.final_validate as fv
from esphome.types import ConfigType

from .. import CONF_PT2323_ID, DUMP_PUBLISHER, PT2323, STATE_PUBLISHER

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["pt2323"]


CONFIG_SCHEMA = select.select_schema(select.ApplySelect).extend(
    {
        cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
        cv.Required(CONF_OPTIONS): cv.All(
            cv.ensure_list(cv.string_strict), cv.Length(min=1, max=5)
        ),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PT2323_ID])
    selects = select.sub_selects(config, parent=hub)
    var = await selects.create(
        config, None, options=config[CONF_OPTIONS], apply="set_input(index)"
    )
    parent = automation.parent_ref(hub)
    entity = automation.parent_ref(var)
    STATE_PUBLISHER.add(
        hub,
        cg.RawStatement(
            f"""{{
  const size_t index = {parent}->get_input();
  if (index >= {entity}->size()) {{
    ESPHOME_LOG_TAG(TAG, "pt2323.select");
    ESP_LOGW(TAG, "Input %u has no configured option", static_cast<unsigned>(index));
  }} else if (!{entity}->has_state() || {entity}->active_index() != index) {{
    {entity}->publish_state(index);
  }}
}}"""
        ),
    )
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SELECT("", "PT2323 Select", {entity});')
    )


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
