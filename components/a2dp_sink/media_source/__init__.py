import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import media_source
import esphome.final_validate as fv
from esphome.const import CONF_ID
from .. import A2DPSink, CONF_A2DP_SINK_ID

DEPENDENCIES = ["a2dp_sink"]
CONFIG_SCHEMA = media_source.media_source_schema(A2DPSink).extend({
    cv.GenerateID(CONF_A2DP_SINK_ID): cv.use_id(A2DPSink),
})


def final_validate(config):
    full = fv.full_config.get()
    sources = [s for s in full.get("media_source", []) if s.get("platform") == "a2dp_sink"]
    if len(sources) != 1:
        raise cv.Invalid("Configure exactly one a2dp_sink media source")
    uses = sum(
        any(str(s) == str(config[CONF_ID]) for s in player.get(key, {}).get("sources", []))
        for player in full.get("media_player", [])
        if player.get("platform") == "speaker_source"
        for key in ("media_pipeline", "announcement_pipeline")
    )
    if uses != 1:
        raise cv.Invalid("Assign the A2DP media source to exactly one speaker_source pipeline")
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    parent = await cg.get_variable(config[CONF_A2DP_SINK_ID])
    await media_source.register_media_source(parent, config)
