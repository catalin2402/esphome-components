import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_FREQUENCY,
    CONF_CLK_PIN,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_CS_PIN,
)

AUTO_LOAD = ["spi"]
CONF_D0_PIN = "d0_pin"
CONF_MODULE_NUMBER = "module_number"

cc1101_ns = cg.esphome_ns.namespace("cc1101")

CONF_BANDWIDTH = "bandwidth"

cc1101 = cc1101_ns.class_("CC1101", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(cc1101),
        cv.Required(CONF_CLK_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_MISO_PIN): pins.gpio_input_pin_schema,
        cv.Required(CONF_MOSI_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_D0_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_MODULE_NUMBER, default=0): cv.uint8_t,
        cv.Optional(CONF_BANDWIDTH, default="200kHz"): cv.frequency,
        cv.Optional(CONF_FREQUENCY, default="433.92MHz"): cv.frequency,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cg.add_library("SmartRC-CC1101-Driver-Lib", "2.5.7")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    CLK_PIN = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    MISO_PIN = await cg.gpio_pin_expression(config[CONF_MISO_PIN])
    MOSI_PIN = await cg.gpio_pin_expression(config[CONF_MOSI_PIN])
    CS_PIN = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    D0_PIN = await cg.gpio_pin_expression(config[CONF_D0_PIN])
    var.set_spi_pins(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN)
    var.set_d0_pin(D0_PIN)
    var.set_module_number(config[CONF_MODULE_NUMBER])
    var.set_bandwidth(config[CONF_BANDWIDTH])
    var.set_frequency(config[CONF_FREQUENCY])
    
   
