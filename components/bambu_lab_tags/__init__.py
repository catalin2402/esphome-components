from esphome import automation, final_validate as fv
import esphome.codegen as cg
from esphome.components import esp32, mifare_classic
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ON_TAG, Framework
from esphome.types import ConfigType

from ._publisher import Publisher

AUTO_LOAD = ["mifare_classic"]
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
DOMAIN = "bambu_lab_tags"
MULTI_CONF = True

CONF_BAMBU_LAB_TAGS_ID = "bambu_lab_tags_id"
CONF_READER_ID = "reader_id"

bambu_lab_tags_ns = cg.esphome_ns.namespace(DOMAIN)
BambuLabTags = bambu_lab_tags_ns.class_(
    "BambuLabTags", cg.Component, mifare_classic.MifareClassicListener
)
Spool = bambu_lab_tags_ns.struct("Spool")
SpoolRef = Spool.operator("ref").operator("const")
TAG_PUBLISHER = Publisher(
    "tag", cg.void, [(SpoolRef, "spool")], define="USE_BAMBU_LAB_TAGS_TAG_PUBLISHER"
)
SPOOL = TAG_PUBLISHER.param("spool")
DUMP_PUBLISHER = Publisher("dump", cg.void, [(cg.const_char_ptr, "TAG")])

_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_TAG, "add_on_tag_callback", [(SpoolRef, "spool")]
    ),
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BambuLabTags),
            cv.GenerateID(CONF_READER_ID): cv.use_id(
                mifare_classic.MifareClassicReader
            ),
            cv.Optional(CONF_ON_TAG): automation.validate_synchronous_automation(
                "bambu_lab_tags.on_tag", "the referenced spool data", extra_schema={}
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    cv.only_with_framework(Framework.ESP_IDF),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    reader = await cg.get_variable(config[CONF_READER_ID])
    cg.add(reader.set_mifare_classic_listener(var))
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_HKDF_C", True)
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_MD_C", True)
    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


def _validate_readers(config: ConfigType) -> ConfigType:
    entries = fv.full_config.get().get(DOMAIN, [])
    readers = [str(entry[CONF_READER_ID]) for entry in entries]
    if len(readers) != len(set(readers)):
        raise cv.Invalid("Only one bambu_lab_tags instance can use each reader")
    return config


FINAL_VALIDATE_SCHEMA = _validate_readers
