from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import (
    CONF_AUTO_CLEAR_ENABLED,
    CONF_BIT_DEPTH,
    CONF_BRIGHTNESS,
    CONF_CLK_PIN,
    CONF_GAMMA_CORRECT,
    CONF_ID,
    CONF_LAMBDA,
    CONF_OE_PIN,
    CONF_ROTATION,
    CONF_TRIGGER_ID,
    CONF_UPDATE_INTERVAL,
)
import esphome.final_validate as fv
from esphome.types import ConfigType

from . import HUB75Display, hub75_icn2053_ns as hub75_ns

CONF_ON_BRIGHTNESS = "on_brightness"
CONF_ON_READY = "on_ready"
CONF_ON_ERROR = "on_error"

CODEOWNERS = ["@catalin2402"]

DEPENDENCIES = ["esp32"]

# Panel dimensions
CONF_PANEL_WIDTH = "panel_width"
CONF_PANEL_HEIGHT = "panel_height"

# Multi-panel layout
CONF_LAYOUT_ROWS = "layout_rows"
CONF_LAYOUT_COLS = "layout_cols"
CONF_LAYOUT = "layout"

# Panel hardware
CONF_SCAN_WIRING = "scan_wiring"
CONF_SHIFT_DRIVER = "shift_driver"

# RGB pins
CONF_R1_PIN = "r1_pin"
CONF_G1_PIN = "g1_pin"
CONF_B1_PIN = "b1_pin"
CONF_R2_PIN = "r2_pin"
CONF_G2_PIN = "g2_pin"
CONF_B2_PIN = "b2_pin"

# Address pins
CONF_A_PIN = "a_pin"
CONF_B_PIN = "b_pin"
CONF_C_PIN = "c_pin"
CONF_D_PIN = "d_pin"
CONF_E_PIN = "e_pin"

# Control pins
CONF_LAT_PIN = "lat_pin"

NEVER = 4294967295  # uint32_t max - value used when update_interval is "never"

# Pin order matches the hub75_pins_t C++ struct.
PIN_KEYS = (
    CONF_R1_PIN,
    CONF_G1_PIN,
    CONF_B1_PIN,
    CONF_R2_PIN,
    CONF_G2_PIN,
    CONF_B2_PIN,
    CONF_A_PIN,
    CONF_B_PIN,
    CONF_C_PIN,
    CONF_D_PIN,
    CONF_E_PIN,
    CONF_LAT_PIN,
    CONF_OE_PIN,
    CONF_CLK_PIN,
)

# Configuration
CONF_CLOCK_SPEED = "clock_speed"
CONF_LATCH_BLANKING = "latch_blanking"
CONF_CLOCK_PHASE = "clock_phase"
CONF_DOUBLE_BUFFER = "double_buffer"
CONF_MIN_REFRESH_RATE = "min_refresh_rate"

# Hardware and layout enums.
Hub75ShiftDriver = hub75_ns.enum("Hub75ShiftDriver", is_class=True)
SHIFT_DRIVERS = {
    "ICN2053": Hub75ShiftDriver.ICN2053,
}

Hub75PanelLayout = hub75_ns.enum("Hub75PanelLayout", is_class=True)
PANEL_LAYOUTS = {
    "HORIZONTAL": Hub75PanelLayout.HORIZONTAL,
    "TOP_LEFT_DOWN": Hub75PanelLayout.TOP_LEFT_DOWN,
    "TOP_RIGHT_DOWN": Hub75PanelLayout.TOP_RIGHT_DOWN,
    "BOTTOM_LEFT_UP": Hub75PanelLayout.BOTTOM_LEFT_UP,
    "BOTTOM_RIGHT_UP": Hub75PanelLayout.BOTTOM_RIGHT_UP,
    "TOP_LEFT_DOWN_ZIGZAG": Hub75PanelLayout.TOP_LEFT_DOWN_ZIGZAG,
    "TOP_RIGHT_DOWN_ZIGZAG": Hub75PanelLayout.TOP_RIGHT_DOWN_ZIGZAG,
    "BOTTOM_LEFT_UP_ZIGZAG": Hub75PanelLayout.BOTTOM_LEFT_UP_ZIGZAG,
    "BOTTOM_RIGHT_UP_ZIGZAG": Hub75PanelLayout.BOTTOM_RIGHT_UP_ZIGZAG,
}

Hub75ScanWiring = hub75_ns.enum("Hub75ScanWiring", is_class=True)
SCAN_WIRINGS = {
    "STANDARD_TWO_SCAN": Hub75ScanWiring.STANDARD_TWO_SCAN,
    "SCAN_1_4_16PX_HIGH": Hub75ScanWiring.SCAN_1_4_16PX_HIGH,
    "SCAN_1_8_32PX_HIGH": Hub75ScanWiring.SCAN_1_8_32PX_HIGH,
    "SCAN_1_8_32PX_FULL": Hub75ScanWiring.SCAN_1_8_32PX_FULL,
    "SCAN_1_8_40PX_HIGH": Hub75ScanWiring.SCAN_1_8_40PX_HIGH,
    "SCAN_1_8_64PX_HIGH": Hub75ScanWiring.SCAN_1_8_64PX_HIGH,
}

Hub75ClockSpeed = hub75_ns.enum("Hub75ClockSpeed", is_class=True)
CLOCK_SPEEDS = {
    "1MHZ": Hub75ClockSpeed.HZ_1M,
    "5MHZ": Hub75ClockSpeed.HZ_5M,
    "8MHZ": Hub75ClockSpeed.HZ_8M,
    "10MHZ": Hub75ClockSpeed.HZ_10M,
    "16MHZ": Hub75ClockSpeed.HZ_16M,
    "20MHZ": Hub75ClockSpeed.HZ_20M,
}


def _validate_config(config: ConfigType) -> ConfigType:
    """Validate driver and layout requirements."""
    errs: list[cv.Invalid] = []

    # Validate layout configuration (validate effective config including C++ defaults)
    layout = config.get(CONF_LAYOUT, "HORIZONTAL")
    layout_rows = config.get(CONF_LAYOUT_ROWS, 1)
    layout_cols = config.get(CONF_LAYOUT_COLS, 1)
    is_zigzag = "ZIGZAG" in layout

    # Single panel (1x1) should use HORIZONTAL
    if layout_rows == 1 and layout_cols == 1 and layout != "HORIZONTAL":
        errs.append(
            cv.Invalid(
                f"Single panel (layout_rows=1, layout_cols=1) should use 'layout: HORIZONTAL' (got {layout})",
                path=[CONF_LAYOUT],
            )
        )

    # HORIZONTAL layout requires single row
    if layout == "HORIZONTAL" and layout_rows != 1:
        errs.append(
            cv.Invalid(
                f"HORIZONTAL layout requires 'layout_rows: 1' (got {layout_rows}). "
                "For multi-row grids, use TOP_LEFT_DOWN or other grid layouts.",
                path=[CONF_LAYOUT_ROWS],
            )
        )

    # Serpentine layouts (non-ZIGZAG) require multiple rows
    # Serpentine physically rotates alternate rows upside down (Y-coordinate inversion)
    # Single-row chains should use HORIZONTAL or ZIGZAG variants
    if not is_zigzag and layout != "HORIZONTAL" and layout_rows == 1:
        errs.append(
            cv.Invalid(
                f"Serpentine layout '{layout}' requires layout_rows > 1 "
                f"(got layout_rows={layout_rows}). "
                "Serpentine wiring physically rotates alternate rows upside down. "
                "For single-row chains, use 'layout: HORIZONTAL' or add '_ZIGZAG' suffix.",
                path=[CONF_LAYOUT_ROWS],
            )
        )

    # ZIGZAG layouts require actual grid (both rows AND cols > 1)
    if is_zigzag and (layout_rows == 1 or layout_cols == 1):
        errs.append(
            cv.Invalid(
                f"ZIGZAG layout '{layout}' requires both layout_rows > 1 AND layout_cols > 1 "
                f"(got rows={layout_rows}, cols={layout_cols}). "
                "For single row/column chains, use non-zigzag layouts or HORIZONTAL.",
                path=[CONF_LAYOUT],
            )
        )

    if errs:
        raise cv.MultipleInvalid(errs)

    return config


def _final_validate(config: ConfigType) -> ConfigType:
    """Validate requirements when using HUB75 display."""
    full_config = fv.full_config.get()
    if (
        sum(
            item.get("platform") == "hub75_icn2053"
            for item in full_config.get("display", [])
        )
        > 1
    ):
        raise cv.Invalid(
            "Only one hub75_icn2053 output is supported; chain panels using layout_rows/layout_cols"
        )
    errs: list[cv.Invalid] = []

    # LVGL-specific validation
    if any(
        str(display_id) == str(config[CONF_ID])
        for lvgl_config in full_config.get("lvgl", [])
        for display_id in lvgl_config.get("displays", [])
    ):
        # Check update_interval (converted from "never" to NEVER constant)
        update_interval = config.get(CONF_UPDATE_INTERVAL)
        if update_interval is not None:
            # Handle both integer (NEVER) and time object cases
            interval_ms = (
                update_interval
                if isinstance(update_interval, int)
                else update_interval.total_milliseconds
            )
            if interval_ms != NEVER:
                errs.append(
                    cv.Invalid(
                        "HUB75 display with LVGL must have 'update_interval: never'. "
                        "LVGL manages its own refresh timing.",
                        path=[CONF_UPDATE_INTERVAL],
                    )
                )

        # Check auto_clear_enabled
        auto_clear = config[CONF_AUTO_CLEAR_ENABLED]
        if auto_clear is not False:
            errs.append(
                cv.Invalid(
                    f"HUB75 display with LVGL must have 'auto_clear_enabled: false' (got '{auto_clear}'). "
                    "LVGL manages screen clearing.",
                    path=[CONF_AUTO_CLEAR_ENABLED],
                )
            )

        # Check double_buffer (C++ default: false)
        double_buffer = config.get(CONF_DOUBLE_BUFFER, False)
        if double_buffer is not False:
            errs.append(
                cv.Invalid(
                    f"HUB75 display with LVGL must have 'double_buffer: false' (got '{double_buffer}'). "
                    "LVGL uses its own buffering strategy.",
                    path=[CONF_DOUBLE_BUFFER],
                )
            )

    if errs:
        raise cv.MultipleInvalid(errs)

    return config


FINAL_VALIDATE_SCHEMA = cv.Schema(_final_validate)


def _validate_icn2053(config):
    from esphome.components.esp32 import get_esp32_variant
    from esphome.components.esp32.const import VARIANT_ESP32
    from esphome.core import CORE

    if CORE.target_framework != "esp-idf":
        raise cv.Invalid("hub75_icn2053 requires framework type: esp-idf")
    if get_esp32_variant() != VARIANT_ESP32:
        raise cv.Invalid("The ICN2053 I2S backend supports the original ESP32 only")
    if config[CONF_PANEL_WIDTH] % 16:
        raise cv.Invalid(
            "panel_width must be a multiple of 16", path=[CONF_PANEL_WIDTH]
        )
    if not 2 <= config[CONF_PANEL_HEIGHT] <= 64 or config[CONF_PANEL_HEIGHT] % 2:
        raise cv.Invalid(
            "panel_height must be even and between 2 and 64 (standard two-scan wiring)",
            path=[CONF_PANEL_HEIGHT],
        )
    if config[CONF_PANEL_HEIGHT] > 16 and CONF_D_PIN not in config:
        raise cv.Invalid(
            "Panels taller than 16 pixels require d_pin", path=[CONF_D_PIN]
        )
    if config[CONF_PANEL_HEIGHT] > 32 and CONF_E_PIN not in config:
        raise cv.Invalid(
            "Panels taller than 32 pixels require e_pin", path=[CONF_E_PIN]
        )
    width = (
        config[CONF_PANEL_WIDTH]
        * config.get(CONF_LAYOUT_COLS, 1)
        * config.get(CONF_LAYOUT_ROWS, 1)
    )
    if (
        width > 32752
        or config[CONF_PANEL_HEIGHT] * config.get(CONF_LAYOUT_ROWS, 1) > 32767
    ):
        raise cv.Invalid("Panel chain exceeds backend coordinate limits")
    if config.get(CONF_SCAN_WIRING, "STANDARD_TWO_SCAN") != "STANDARD_TWO_SCAN":
        raise cv.Invalid(
            "ICN2053 currently supports STANDARD_TWO_SCAN only", path=[CONF_SCAN_WIRING]
        )
    for key in (CONF_BIT_DEPTH, CONF_LATCH_BLANKING, CONF_MIN_REFRESH_RATE):
        if key in config:
            raise cv.Invalid(
                f"{key} is not configurable in the ICN2053 backend", path=[key]
            )
    if config.get(CONF_DOUBLE_BUFFER, False):
        raise cv.Invalid(
            "The ICN2053 backend uses a single RGB565 framebuffer; use double_buffer: false",
            path=[CONF_DOUBLE_BUFFER],
        )
    if str(config.get(CONF_CLOCK_SPEED, "1MHZ")) == "16MHZ":
        raise cv.Invalid(
            "16MHZ cannot be generated exactly by this I2S backend; use 8MHZ, 10MHZ or 20MHZ",
            path=[CONF_CLOCK_SPEED],
        )
    seen = set()
    for key in PIN_KEYS:
        if key not in config:
            continue
        pin = config[key]
        if pin.get("inverted", False) or pin["mode"].get("open_drain", False):
            raise cv.Invalid(
                "DMA pins must be non-inverted push-pull outputs", path=[key]
            )
        number = pin["number"]
        if number in seen:
            raise cv.Invalid("HUB75 pins must be unique", path=[key])
        seen.add(number)
    return config


BrightnessTrigger = hub75_ns.class_(
    "BrightnessTrigger", automation.Trigger.template(cg.int_)
)
ReadyTrigger = hub75_ns.class_("ReadyTrigger", automation.Trigger.template())
ErrorTrigger = hub75_ns.class_("ErrorTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(HUB75Display),
            cv.Optional(CONF_ON_BRIGHTNESS): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BrightnessTrigger)}
            ),
            cv.Optional(CONF_ON_READY): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ReadyTrigger)}
            ),
            cv.Optional(CONF_ON_ERROR): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ErrorTrigger)}
            ),
            cv.Optional(CONF_ROTATION): display.validate_rotation,
            # Panel dimensions
            cv.Required(CONF_PANEL_WIDTH): cv.positive_int,
            cv.Required(CONF_PANEL_HEIGHT): cv.positive_int,
            # Multi-panel layout
            cv.Optional(CONF_LAYOUT_ROWS): cv.positive_int,
            cv.Optional(CONF_LAYOUT_COLS): cv.positive_int,
            cv.Optional(CONF_LAYOUT): cv.enum(PANEL_LAYOUTS, upper=True, space="_"),
            # Panel hardware configuration
            cv.Optional(CONF_SCAN_WIRING): cv.enum(SCAN_WIRINGS, upper=True),
            cv.Optional(CONF_SHIFT_DRIVER, default="ICN2053"): cv.enum(
                SHIFT_DRIVERS, upper=True
            ),
            # Display configuration
            cv.Optional(CONF_DOUBLE_BUFFER): cv.boolean,
            cv.Optional(CONF_BRIGHTNESS): cv.int_range(min=0, max=255),
            cv.Optional(CONF_BIT_DEPTH): cv.int_range(min=4, max=12),
            cv.Optional(CONF_GAMMA_CORRECT): cv.enum(
                {"LINEAR": 0, "CIE1931": 1, "GAMMA_2_2": 2}, upper=True
            ),
            cv.Optional(CONF_MIN_REFRESH_RATE): cv.int_range(min=40, max=200),
            # RGB data pins
            cv.Required(CONF_R1_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_G1_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_B1_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_R2_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_G2_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_B2_PIN): pins.internal_gpio_output_pin_schema,
            # Address pins
            cv.Required(CONF_A_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_B_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_C_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_D_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_E_PIN): pins.internal_gpio_output_pin_schema,
            # Control pins
            cv.Required(CONF_LAT_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_OE_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_schema,
            # Timing configuration
            cv.Optional(CONF_CLOCK_SPEED): cv.enum(CLOCK_SPEEDS, upper=True),
            cv.Optional(CONF_LATCH_BLANKING): cv.positive_int,
            cv.Optional(CONF_CLOCK_PHASE): cv.boolean,
        }
    ),
    _validate_config,
    _validate_icn2053,
)


async def to_code(config):
    from esphome.components.esp32 import include_builtin_idf_component

    include_builtin_idf_component("esp_driver_gpio")
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(
        var.set_geometry(
            config[CONF_PANEL_WIDTH],
            config[CONF_PANEL_HEIGHT],
            config.get(CONF_LAYOUT_ROWS, 1),
            config.get(CONF_LAYOUT_COLS, 1),
            config.get(CONF_LAYOUT, PANEL_LAYOUTS["HORIZONTAL"]),
        )
    )
    pin_numbers = []
    for key in PIN_KEYS:
        if key in config:
            pin = await cg.gpio_pin_expression(config[key])
            pin_numbers.append(
                cg.RawExpression(f"static_cast<int8_t>({pin.get_pin()})")
            )
        else:
            pin_numbers.append(-1)
    cg.add(
        var.set_pins(
            cg.RawExpression("{" + ", ".join(str(n) for n in pin_numbers) + "}")
        )
    )
    cg.add(
        var.set_clock(
            config.get(CONF_CLOCK_SPEED, CLOCK_SPEEDS["1MHZ"]),
            config.get(CONF_CLOCK_PHASE, False),
        )
    )
    cg.add(var.set_brightness(config.get(CONF_BRIGHTNESS, 128)))
    if CONF_GAMMA_CORRECT in config:
        cg.add(var.set_gamma(config[CONF_GAMMA_CORRECT]))
    await display.register_display(var, config)
    if CONF_LAMBDA in config:
        writer = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(writer))

    for key, args in (
        (CONF_ON_BRIGHTNESS, [(cg.int_, "brightness")]),
        (CONF_ON_READY, []),
        (CONF_ON_ERROR, []),
    ):
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, args, conf)
