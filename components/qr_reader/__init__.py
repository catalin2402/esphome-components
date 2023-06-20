import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["text_sensor"]

qr_reader_ns = cg.esphome_ns.namespace("qr_reader")
QrReader = qr_reader_ns.class_(
    "QrReader", text_sensor.TextSensor, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema()
    .extend({cv.GenerateID(): cv.declare_id(QrReader)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
