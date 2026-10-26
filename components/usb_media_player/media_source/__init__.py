import esphome.codegen as cg
from esphome.components import media_source
import esphome.config_validation as cv
from esphome.const import CONF_BUFFER_SIZE, CONF_ID
import esphome.final_validate as fv

from .. import CONF_USB_MEDIA_PLAYER_ID, USBMediaPlayer, USBMediaSource

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["usb_media_player"]
AUTO_LOAD = ["audio"]

CONF_AUTO_START = "auto_start"
CONF_OUTPUT_GAIN_DB = "output_gain_db"


def validate_config(config):
    if config[CONF_BUFFER_SIZE] % 4:
        raise cv.Invalid("buffer_size must be a multiple of 4", path=[CONF_BUFFER_SIZE])
    return config


CONFIG_SCHEMA = cv.All(
    media_source.media_source_schema(USBMediaSource)
    .extend(
        {
            cv.GenerateID(CONF_USB_MEDIA_PLAYER_ID): cv.use_id(USBMediaPlayer),
            cv.Optional(CONF_BUFFER_SIZE, default=32768): cv.int_range(
                min=8192, max=262144
            ),
            cv.Optional(CONF_AUTO_START, default=True): cv.boolean,
            cv.Optional(CONF_OUTPUT_GAIN_DB, default=0.0): cv.float_range(
                min=-60.0, max=0.0
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_config,
)


def final_validate(config):
    full_config = fv.full_config.get()
    uses = sum(
        any(
            str(source) == str(config[CONF_ID])
            for source in player.get(key, {}).get("sources", [])
        )
        for player in full_config.get("media_player", [])
        if player.get("platform") == "speaker_source"
        for key in ("media_pipeline", "announcement_pipeline")
    )
    if uses != 1:
        raise cv.Invalid(
            "Assign the USB media source to exactly one speaker_source pipeline"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    cg.add_define("USE_USB_MEDIA_PLAYER_SOURCE")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_source.register_media_source(var, config)
    parent = await cg.get_variable(config[CONF_USB_MEDIA_PLAYER_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_source(var))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))
    cg.add(var.set_auto_start(config[CONF_AUTO_START]))
    cg.add(var.set_output_gain_db(config[CONF_OUTPUT_GAIN_DB]))
