from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core
from esphome.automation import maybe_simple_id
from esphome.components import display, i2c
from esphome.const import (
    CONF_DATA,
    CONF_DIMENSIONS,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGE_ID,
    CONF_PAGES,
    CONF_POSITION,
)
from esphome.types import ConfigType

DOMAIN = "lcd_pcf8574_optimised"
DEPENDENCIES = ["i2c"]

CONF_USER_CHARACTERS = "user_characters"
CONF_AUTO_CYCLE_INTERVAL = "auto_cycle_interval"

lcd_pcf8574_optimised_ns = cg.esphome_ns.namespace("lcd_pcf8574_optimised")
PCF8574OptimisedLCDDisplay = lcd_pcf8574_optimised_ns.class_(
    "PCF8574OptimisedLCDDisplay", cg.PollingComponent, i2c.I2CDevice
)

OptimisedLCDDisplayRef = PCF8574OptimisedLCDDisplay.operator("ref")
OptimisedLCDDisplayPage = lcd_pcf8574_optimised_ns.class_("OptimisedLCDDisplayPage")
OptimisedLCDDisplayPagePtr = OptimisedLCDDisplayPage.operator("ptr")
OptimisedLCDDisplayPageShowAction = lcd_pcf8574_optimised_ns.class_(
    "OptimisedLCDDisplayPageShowAction", automation.Action
)


def validate_lcd_dimensions(value: Any) -> list[int]:
    value = cv.dimensions(value)
    if value[0] > 0x40:
        raise cv.Invalid("LCD displays can't have more than 64 columns")
    if value[1] > 4:
        raise cv.Invalid("LCD displays can't have more than 4 rows")
    return value


def validate_user_characters(value: list[ConfigType]) -> list[ConfigType]:
    positions = set()
    for conf in value:
        if conf[CONF_POSITION] in positions:
            raise cv.Invalid(
                f"Duplicate user defined character at position {conf[CONF_POSITION]}"
            )
        positions.add(conf[CONF_POSITION])
    return value


def validate_auto_cycle(config: ConfigType) -> ConfigType:
    if config[CONF_AUTO_CYCLE_INTERVAL].total_milliseconds and CONF_PAGES not in config:
        raise cv.Invalid(
            "auto_cycle_interval requires pages", path=[CONF_AUTO_CYCLE_INTERVAL]
        )
    return config


LCD_SCHEMA = display.BASIC_DISPLAY_SCHEMA.extend(
    {
        cv.Required(CONF_DIMENSIONS): validate_lcd_dimensions,
        cv.Optional(CONF_USER_CHARACTERS): cv.All(
            cv.ensure_list(
                cv.Schema(
                    {
                        cv.Required(CONF_POSITION): cv.int_range(min=0, max=7),
                        cv.Required(CONF_DATA): cv.All(
                            cv.ensure_list(cv.int_range(min=0, max=31)),
                            cv.Length(min=8, max=8),
                        ),
                    }
                ),
            ),
            cv.Length(max=8),
            validate_user_characters,
        ),
        cv.Optional(CONF_PAGES): cv.All(
            cv.ensure_list(
                {
                    cv.GenerateID(): cv.declare_id(OptimisedLCDDisplayPage),
                    cv.Required(CONF_LAMBDA): cv.lambda_,
                }
            ),
            cv.Length(min=1),
        ),
        cv.Optional(
            CONF_AUTO_CYCLE_INTERVAL, default="0s"
        ): cv.positive_time_period_milliseconds,
    }
).extend(cv.polling_component_schema("1s"))
LCD_SCHEMA.add_extra(cv.has_at_most_one_key(CONF_LAMBDA, CONF_PAGES))
LCD_SCHEMA.add_extra(validate_auto_cycle)


CONFIG_SCHEMA = LCD_SCHEMA.extend(
    {cv.GenerateID(): cv.declare_id(PCF8574OptimisedLCDDisplay)}
).extend(i2c.i2c_device_schema(0x3F))


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    # LCD pages have a different writer type from graphical display pages.
    await display.register_display(
        var, {key: value for key, value in config.items() if key != CONF_PAGES}
    )
    columns, rows = config[CONF_DIMENSIONS]
    buffer = cg.static_buffer(config[CONF_ID], cg.uint8, columns * rows)
    cg.add(var.set_dimensions(columns, rows, buffer))
    previous = cg.static_buffer(config[CONF_ID], cg.uint8, columns * rows)
    cg.add(var.set_previous_buffer(previous))
    if CONF_USER_CHARACTERS in config:
        for usr in config[CONF_USER_CHARACTERS]:
            cg.add(var.set_user_defined_char(usr[CONF_POSITION], usr[CONF_DATA]))
    if CONF_PAGES in config:
        pages = []
        for conf in config[CONF_PAGES]:
            lambda_ = await cg.process_lambda(
                conf[CONF_LAMBDA], [(OptimisedLCDDisplayRef, "it")], return_type=cg.void
            )
            page = cg.new_Pvariable(conf[CONF_ID], lambda_)
            pages.append(page)
        cg.add(var.set_pages(pages))
    if (auto_cycle_interval := config.get(CONF_AUTO_CYCLE_INTERVAL)) is not None:
        cg.add(var.set_auto_cycle_interval(auto_cycle_interval.total_milliseconds))

    await i2c.register_i2c_device(var, config)
    await display.add_writer(var, config, PCF8574OptimisedLCDDisplay.operator("ref"))


@automation.register_action(
    "lcd_pcf8574_optimised.page.show",
    OptimisedLCDDisplayPageShowAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.templatable(cv.use_id(OptimisedLCDDisplayPage)),
        }
    ),
    synchronous=True,
)
async def display_page_show_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    if isinstance(config[CONF_ID], core.Lambda):
        template_ = await cg.templatable(config[CONF_ID], args, OptimisedLCDDisplayPagePtr)
        cg.add(var.set_page(template_))
    else:
        paren = await cg.get_variable(config[CONF_ID])
        template_ = await cg.templatable(paren, args, OptimisedLCDDisplayPagePtr)
        cg.add(var.set_page(template_))
    return var


LCD_PAGE_CYCLE_ACTION_SCHEMA = maybe_simple_id(
    {cv.GenerateID(CONF_ID): cv.use_id(PCF8574OptimisedLCDDisplay)}
)

automation.register_apply_action(
    "lcd_pcf8574_optimised.page.show_next",
    LCD_PAGE_CYCLE_ACTION_SCHEMA,
    automation.ApplyCall("show_next_page()"),
)

automation.register_apply_action(
    "lcd_pcf8574_optimised.page.show_previous",
    LCD_PAGE_CYCLE_ACTION_SCHEMA,
    automation.ApplyCall("show_prev_page()"),
)

automation.register_apply_condition(
    "lcd_pcf8574_optimised.is_displaying_page",
    cv.maybe_simple_value(
        {
            cv.GenerateID(CONF_ID): cv.use_id(PCF8574OptimisedLCDDisplay),
            cv.Required(CONF_PAGE_ID): cv.use_id(OptimisedLCDDisplayPage),
        },
        key=CONF_PAGE_ID,
    ),
    automation.ApplyCall(
        "get_active_page() == {}", ((CONF_PAGE_ID, OptimisedLCDDisplayPagePtr),)
    ),
)
