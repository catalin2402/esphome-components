import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart, text_sensor, binary_sensor
from esphome.automation import maybe_simple_id
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

esp_player_ns = cg.esphome_ns.namespace('esp_player')
EspPlayer = esp_player_ns.class_('EspPlayer', cg.Component, uart.UARTDevice)

PlayPauseAction = esp_player_ns.class_("PlayPauseAction", automation.Action)

CONF_ESP_PLAYER_ID = 'esp_player'
CONF_CURRENT_SONG_SENSOR = 'current_song_sensor'
CONF_PLAY_STATE_SENSOR = "play_state_sensor"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EspPlayer),
    cv.Optional(CONF_CURRENT_SONG_SENSOR): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_PLAY_STATE_SENSOR): binary_sensor.binary_sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

PLAYER_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(EspPlayer),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_CURRENT_SONG_SENSOR in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CURRENT_SONG_SENSOR])
        cg.add(var.set_current_song_sensor(sens))
    
    if CONF_PLAY_STATE_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_PLAY_STATE_SENSOR])
        cg.add(var.set_play_state_sensor(sens))

    await uart.register_uart_device(var, config)

@automation.register_action("esp_player.play_pause", PlayPauseAction, PLAYER_ACTION_SCHEMA)
async def esp_player_play_pause_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)