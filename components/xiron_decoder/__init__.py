import esphome.config_validation as cv
import esphome.codegen as cg

CODEOWNERS = ["@catalin2402"]
xiron_decoder_ns = cg.esphome_ns.namespace("xiron_decoder")
XironDecoder = xiron_decoder_ns.class_("XironDecoder", cg.Component)

CONFIG_SCHEMA = cv.Schema({}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    pass
