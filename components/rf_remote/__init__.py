import esphome.codegen as cg
import esphome.config_validation as cv

CODEOWNERS = ["@catalin2402"]

rf_remote_ns = cg.esphome_ns.namespace("rf_remote")

RFRemoteBase = rf_remote_ns.class_("RFRemoteBase", cg.Component)
RFRemoteSensor = rf_remote_ns.class_("RFRemoteSensor", RFRemoteBase)
RFRemoteBinarySensor = rf_remote_ns.class_("RFRemoteBinarySensor", RFRemoteBase)

CONFIG_SCHEMA = cv.Schema({}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    pass
