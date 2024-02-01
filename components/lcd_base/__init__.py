import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, automation
from esphome.automation import maybe_simple_id
from esphome.components import display
from esphome.const import (
    CONF_DIMENSIONS,
    CONF_POSITION,
    CONF_DATA,
    CONF_PAGES,
    CONF_LAMBDA,
    CONF_ID,
    CONF_PAGE_ID,
)

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
LCDDisplayPageShowNextAction = lcd_base_ns.class_(
    "LCDDisplayPageShowNextAction", automation.Action
)
LCDDisplayPageShowPrevAction = lcd_base_ns.class_(
    "LCDDisplayPageShowPrevAction", automation.Action
)
LCDDisplayIsDisplayingPageCondition = lcd_base_ns.class_(
    "LCDDisplayIsDisplayingPageCondition", automation.Condition
)
LCDDisplayOnPageChangeTrigger = lcd_base_ns.class_(
    "LCDDisplayOnPageChangeTrigger", automation.Trigger
)


def validate_lcd_dimensions(value):
    value = cv.dimensions(value)
    if value[0] > 0x40:
        raise cv.Invalid("LCD displays can't have more than 64 columns")
    if value[1] > 4:
        raise cv.Invalid("LCD displays can't have more than 4 rows")
    return value


def validate_user_characters(value):
    positions = set()
    for conf in value:
        if conf[CONF_POSITION] in positions:
            raise cv.Invalid(
                f"Duplicate user defined character at position {conf[CONF_POSITION]}"
            )
        positions.add(conf[CONF_POSITION])
    return value


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
        cv.Optional(CONF_AUTO_CYCLE_INTERVAL, default="0s" ): cv.positive_time_period_milliseconds
    }
).extend(cv.polling_component_schema("1s"))


async def setup_lcd_display(var, config):
    await cg.register_component(var, config)
    cg.add(var.set_dimensions(config[CONF_DIMENSIONS][0], config[CONF_DIMENSIONS][1]))
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
        cg.add(var.set_auto_cycle_interval(auto_cycle_interval))


@automation.register_action(
    "lcddisplay.page.show",
    LCDDisplayPageShowAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.templatable(cv.use_id(LCDDisplayPage)),
        }
    ),
)
async def display_page_show_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    if isinstance(config[CONF_ID], core.Lambda):
        template_ = await cg.templatable(config[CONF_ID], args, LCDDisplayPagePtr)
        cg.add(var.set_page(template_))
    else:
        paren = await cg.get_variable(config[CONF_ID])
        cg.add(var.set_page(paren))
    return var


@automation.register_action(
    "lcddisplay.page.show_next",
    LCDDisplayPageShowNextAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.templatable(cv.use_id(LCDDisplay)),
        }
    ),
)
async def display_page_show_next_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "lcddisplay.page.show_previous",
    LCDDisplayPageShowPrevAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.templatable(cv.use_id(LCDDisplay)),
        }
    ),
)
async def display_page_show_previous_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_condition(
    "lcddisplay.is_displaying_page",
    LCDDisplayIsDisplayingPageCondition,
    cv.maybe_simple_value(
        {
            cv.GenerateID(CONF_ID): cv.use_id(LCDDisplay),
            cv.Required(CONF_PAGE_ID): cv.use_id(LCDDisplayPage),
        },
        key=CONF_PAGE_ID,
    ),
)
async def display_is_displaying_page_to_code(config, condition_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    page = await cg.get_variable(config[CONF_PAGE_ID])
    var = cg.new_Pvariable(condition_id, template_arg, paren)
    cg.add(var.set_page(page))

    return var
