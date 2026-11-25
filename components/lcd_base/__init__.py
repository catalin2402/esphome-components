from typing import Any

from esphome import automation, core
from esphome.automation import maybe_simple_id
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import (
    CONF_DATA,
    CONF_DIMENSIONS,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGE_ID,
    CONF_PAGES,
    CONF_POSITION,
)
from esphome.cpp_generator import MockObj
from esphome.types import ConfigType

DOMAIN = "lcd_base"

CONF_USER_CHARACTERS = "user_characters"
CONF_AUTO_CYCLE_INTERVAL = "auto_cycle_interval"

lcd_base_ns = cg.esphome_ns.namespace("lcd_base")
LCDDisplay = lcd_base_ns.class_("LCDDisplay", cg.PollingComponent)

LCDDisplayRef = LCDDisplay.operator("ref")
LCDDisplayPage = lcd_base_ns.class_("LCDDisplayPage")
LCDDisplayPagePtr = LCDDisplayPage.operator("ptr")
LCDDisplayPageShowAction = lcd_base_ns.class_(
    "LCDDisplayPageShowAction", automation.Action
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
                    cv.GenerateID(): cv.declare_id(LCDDisplayPage),
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


async def setup_lcd_display(var: MockObj, config: ConfigType) -> None:
    # LCD pages have a different writer type from graphical display pages.
    await display.register_display(
        var, {key: value for key, value in config.items() if key != CONF_PAGES}
    )
    columns, rows = config[CONF_DIMENSIONS]
    buffer = cg.static_buffer(config[CONF_ID], cg.uint8, columns * rows)
    cg.add(var.set_dimensions(columns, rows, buffer))
    if CONF_USER_CHARACTERS in config:
        for usr in config[CONF_USER_CHARACTERS]:
            cg.add(var.set_user_defined_char(usr[CONF_POSITION], usr[CONF_DATA]))
    if CONF_PAGES in config:
        pages = []
        for conf in config[CONF_PAGES]:
            lambda_ = await cg.process_lambda(
                conf[CONF_LAMBDA], [(LCDDisplayRef, "it")], return_type=cg.void
            )
            page = cg.new_Pvariable(conf[CONF_ID], lambda_)
            pages.append(page)
        cg.add(var.set_pages(pages))
    if (auto_cycle_interval := config.get(CONF_AUTO_CYCLE_INTERVAL)) is not None:
        cg.add(var.set_auto_cycle_interval(auto_cycle_interval.total_milliseconds))


@automation.register_action(
    "lcddisplay.page.show",
    LCDDisplayPageShowAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.templatable(cv.use_id(LCDDisplayPage)),
        }
    ),
    synchronous=True,
)
async def display_page_show_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    if isinstance(config[CONF_ID], core.Lambda):
        template_ = await cg.templatable(config[CONF_ID], args, LCDDisplayPagePtr)
        cg.add(var.set_page(template_))
    else:
        paren = await cg.get_variable(config[CONF_ID])
        template_ = await cg.templatable(paren, args, LCDDisplayPagePtr)
        cg.add(var.set_page(template_))
    return var


LCD_PAGE_CYCLE_ACTION_SCHEMA = maybe_simple_id(
    {cv.GenerateID(CONF_ID): cv.use_id(LCDDisplay)}
)

automation.register_apply_action(
    "lcddisplay.page.show_next",
    LCD_PAGE_CYCLE_ACTION_SCHEMA,
    automation.ApplyCall("show_next_page()"),
)

automation.register_apply_action(
    "lcddisplay.page.show_previous",
    LCD_PAGE_CYCLE_ACTION_SCHEMA,
    automation.ApplyCall("show_prev_page()"),
)

automation.register_apply_condition(
    "lcddisplay.is_displaying_page",
    cv.maybe_simple_value(
        {
            cv.GenerateID(CONF_ID): cv.use_id(LCDDisplay),
            cv.Required(CONF_PAGE_ID): cv.use_id(LCDDisplayPage),
        },
        key=CONF_PAGE_ID,
    ),
    automation.ApplyCall(
        "get_active_page() == {}", ((CONF_PAGE_ID, LCDDisplayPagePtr),)
    ),
)
