from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.types import ConfigType

from .. import (
    CONF_BAMBU_LAB_TAGS_ID,
    DUMP_PUBLISHER,
    SPOOL,
    TAG_PUBLISHER,
    BambuLabTags,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["bambu_lab_tags"]

TYPES = (
    "UID",
    "MATERIAL_ID",
    "VARIANT_ID",
    "FILAMENT_TYPE",
    "DETAILED_FILAMENT_TYPE",
    "COLOR",
    "PRODUCTION_DATE",
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend(
    {
        cv.GenerateID(CONF_BAMBU_LAB_TAGS_ID): cv.use_id(BambuLabTags),
        cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
    }
)


async def to_code(config: ConfigType) -> None:
    var = await text_sensor.new_text_sensor(config)
    hub = await cg.get_variable(config[CONF_BAMBU_LAB_TAGS_ID])
    entity = parent_ref(var)
    field = "color_name" if config[CONF_TYPE] == "COLOR" else config[CONF_TYPE].lower()
    TAG_PUBLISHER.add(hub, entity.publish_state(getattr(SPOOL, field)))
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_TEXT_SENSOR("", "Bambu Lab Tags", {entity});')
    )
