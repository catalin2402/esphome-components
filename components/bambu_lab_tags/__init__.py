import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import final_validate as fv
from esphome.components import esp32, mifare_classic
from esphome.const import CONF_ID, CONF_ON_TAG, CONF_TRIGGER_ID, Framework

DOMAIN = "bambu_lab_tags"
CONF_BAMBU_LAB_TAGS_ID = "bambu_lab_tags_id"
CONF_READER_ID = "reader_id"

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["mifare_classic"]
MULTI_CONF = True
bambu_lab_tags_ns = cg.esphome_ns.namespace(DOMAIN)
BambuLabTags = bambu_lab_tags_ns.class_("BambuLabTags", cg.Component)
Spool = bambu_lab_tags_ns.struct("Spool")
SpoolRef = Spool.operator("ref").operator("const")
SpoolTrigger = bambu_lab_tags_ns.class_(
    "SpoolTrigger", automation.Trigger.template(SpoolRef)
)
CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BambuLabTags),
            cv.GenerateID(CONF_READER_ID): cv.use_id(
                mifare_classic.MifareClassicReader
            ),
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SpoolTrigger),
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    cv.only_with_framework(Framework.ESP_IDF),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    reader = await cg.get_variable(config[CONF_READER_ID])
    cg.add(reader.set_mifare_classic_listener(var))
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_HKDF_C", True)
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_MD_C", True)
    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(SpoolRef, "spool")], conf)


def _validate_readers(config):
    entries = fv.full_config.get().get(DOMAIN, [])
    readers = [str(entry[CONF_READER_ID]) for entry in entries]
    if len(readers) != len(set(readers)):
        raise cv.Invalid("Only one bambu_lab_tags instance can use each reader")
    return config


FINAL_VALIDATE_SCHEMA = _validate_readers
