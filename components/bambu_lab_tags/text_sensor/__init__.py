import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_TYPE

from .. import CONF_BAMBU_LAB_TAGS_ID, BambuLabTags, bambu_lab_tags_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["bambu_lab_tags"]

BambuLabTagsTextSensor = bambu_lab_tags_ns.class_(
    "BambuLabTagsTextSensor",
    text_sensor.TextSensor,
    cg.Component,
    cg.Parented.template(BambuLabTags),
)
TextSensorType = bambu_lab_tags_ns.enum("TextSensorType", is_class=True)
TYPES = {
    "UID": TextSensorType.UID,
    "MATERIAL_ID": TextSensorType.MATERIAL_ID,
    "VARIANT_ID": TextSensorType.VARIANT_ID,
    "FILAMENT_TYPE": TextSensorType.FILAMENT_TYPE,
    "DETAILED_FILAMENT_TYPE": TextSensorType.DETAILED_FILAMENT_TYPE,
    "COLOR": TextSensorType.COLOR,
    "PRODUCTION_DATE": TextSensorType.PRODUCTION_DATE,
}

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(BambuLabTagsTextSensor)
    .extend(
        {
            cv.GenerateID(CONF_BAMBU_LAB_TAGS_ID): cv.use_id(BambuLabTags),
            cv.Required(CONF_TYPE): cv.enum(TYPES, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_BAMBU_LAB_TAGS_ID])
    cg.add(var.set_type(config[CONF_TYPE]))
