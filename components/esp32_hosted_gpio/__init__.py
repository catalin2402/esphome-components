from esphome import pins
import esphome.codegen as cg
from esphome.components import esp32
import esphome.config_validation as cv
from esphome import final_validate as fv
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OPEN_DRAIN,
    CONF_OUTPUT,
    CONF_PULLDOWN,
    CONF_PULLUP,
)

AUTO_LOAD = ["gpio_expander"]
DEPENDENCIES = ["esp32_hosted"]

esp32_hosted_gpio_ns = cg.esphome_ns.namespace("esp32_hosted_gpio")

ESP32HostedGPIOComponent = esp32_hosted_gpio_ns.class_(
    "ESP32HostedGPIOComponent", cg.Component
)
ESP32HostedGPIOPin = esp32_hosted_gpio_ns.class_("ESP32HostedGPIOPin", cg.GPIOPin)

CONF_ESP32_HOSTED_GPIO = "esp32_hosted_gpio"
CONF_RESERVED_PINS = "reserved_pins"

CONF_BUS_WIDTH = "bus_width"
CONF_TYPE = "type"
CONF_VARIANT = "variant"

# Slave-side pins used by ESPHome's stock ESP-Hosted slave firmware.
# Source: esphome/esp-hosted-firmware builds the stock ESP-Hosted slave example;
# SDIO values are from esp-hosted-mcu/slave/main/Kconfig.projbuild.
SDIO_SLAVE_PINS = {
    "ESP32": {
        "cmd": 15,
        "clk": 14,
        "d0": 2,
        "d1": 4,
        "d2": 12,
        "d3": 13,
    },
    "ESP32C5": {
        "cmd": 10,
        "clk": 9,
        "d0": 8,
        "d1": 7,
        "d2": 14,
        "d3": 13,
    },
    "ESP32C6": {
        "cmd": 18,
        "clk": 19,
        "d0": 20,
        "d1": 21,
        "d2": 22,
        "d3": 23,
    },
    "ESP32C61": {
        "cmd": 25,
        "clk": 26,
        "d0": 27,
        "d1": 28,
        "d2": 22,
        "d3": 23,
    },
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESP32HostedGPIOComponent),
        cv.Optional(CONF_RESERVED_PINS, default=[]): cv.ensure_list(
            cv.int_range(min=0, max=63)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add_define("USE_ESP32_HOSTED_GPIO")
    esp32.add_idf_sdkconfig_option("CONFIG_ESP_HOSTED_ENABLE_GPIO_EXPANDER", True)


def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must include input or output")
    if value[CONF_OPEN_DRAIN] and not value[CONF_OUTPUT]:
        raise cv.Invalid("Open drain requires output mode")
    if value[CONF_PULLUP] and value[CONF_PULLDOWN]:
        raise cv.Invalid("Can only have one of pullup or pulldown")
    return value


def _as_list(value):
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def _slave_transport_pin_map(config):
    pin_map = {}
    transport_type = config.get(CONF_TYPE, "sdio")
    variant = config.get(CONF_VARIANT)
    if variant is None or transport_type != "sdio":
        return pin_map

    pins_for_variant = SDIO_SLAVE_PINS.get(variant, {})
    names = ["cmd", "clk", "d0", "d1"]
    if config.get(CONF_BUS_WIDTH, 4) == 4:
        names.extend(["d2", "d3"])

    for name in names:
        if name in pins_for_variant:
            pin_map[pins_for_variant[name]] = f"{variant} {transport_type.upper()} {name}"
    return pin_map


def _primary_esp32_hosted_config():
    full_config = fv.full_config.get()
    hosted_configs = _as_list(full_config.get("esp32_hosted"))
    return hosted_configs[0] if hosted_configs else {}


def _final_validate(config):
    hosted_config = _primary_esp32_hosted_config()
    if hosted_config.get(CONF_TYPE) == "spi" and not config.get(CONF_RESERVED_PINS):
        raise cv.Invalid(
            "reserved_pins is required when esp32_hosted uses SPI because "
            "the ESP-Hosted slave-side SPI GPIOs are firmware/config dependent "
            "and cannot be inferred. Set reserved_pins to every co-processor "
            "GPIO used by the ESP-Hosted SPI transport: MOSI, MISO, CLK, CS, "
            "handshake, and data_ready, plus any other co-processor GPIOs that "
            "must not be exposed through esp32_hosted_gpio.",
            path=[CONF_RESERVED_PINS],
        )

    interface_pins = _slave_transport_pin_map(hosted_config)

    hub_id = config[CONF_ID]
    for (key, client_id, number), pin_list in pins.PIN_SCHEMA_REGISTRY.pins_used.items():
        if key != CONF_ESP32_HOSTED_GPIO or str(client_id) != str(hub_id):
            continue

        if number in config.get(CONF_RESERVED_PINS, []):
            reason = "reserved by esp32_hosted_gpio reserved_pins"
        elif number in interface_pins:
            reason = f"used by the ESP-Hosted slave transport ({interface_pins[number]})"
        else:
            continue

        for pin_path, _, _ in pin_list:
            raise cv.Invalid(
                f"GPIO{number} is {reason} and cannot be used as an "
                "esp32_hosted_gpio pin",
                path=pin_path,
            )

    return config


FINAL_VALIDATE_SCHEMA = _final_validate


ESP32_HOSTED_GPIO_PIN_SCHEMA = pins.gpio_base_schema(
    ESP32HostedGPIOPin,
    cv.int_range(min=0, max=63),
    modes=[
        CONF_INPUT,
        CONF_OUTPUT,
        CONF_OPEN_DRAIN,
        CONF_PULLUP,
        CONF_PULLDOWN,
    ],
    mode_validator=validate_mode,
    invertible=True,
).extend(
    {
        cv.Required(CONF_ESP32_HOSTED_GPIO): cv.use_id(ESP32HostedGPIOComponent),
    }
)


@pins.PIN_SCHEMA_REGISTRY.register(
    CONF_ESP32_HOSTED_GPIO,
    ESP32_HOSTED_GPIO_PIN_SCHEMA,
)
async def esp32_hosted_gpio_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_ESP32_HOSTED_GPIO])

    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_NUMBER]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
