import esphome.codegen as cg
from esphome.components import binary_sensor, remote_base, sensor

CODEOWNERS = ["@catalin2402"]

rf_remote_ns = cg.esphome_ns.namespace("rf_remote")

RFRemoteBase = rf_remote_ns.class_(
    "RFRemoteBase", remote_base.RemoteReceiverListener, cg.Component
)
RFRemoteSensor = rf_remote_ns.class_(
    "RFRemoteSensor", RFRemoteBase, sensor.Sensor
)
RFRemoteBinarySensor = rf_remote_ns.class_(
    "RFRemoteBinarySensor", RFRemoteBase, binary_sensor.BinarySensor
)
