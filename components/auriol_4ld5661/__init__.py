import esphome.codegen as cg
import esphome.config_validation as cv

auriol_4ld5661_ns = cg.esphome_ns.namespace("auriol_4ld5661")
Auriol4LD5661 = auriol_4ld5661_ns.class_("Auriol4LD5661", cg.Component)

CONFIG_SCHEMA = cv.Schema({}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    pass
