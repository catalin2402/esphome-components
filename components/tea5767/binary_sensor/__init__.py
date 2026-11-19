from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.types import ConfigType

from .. import CONF_TEA5767_ID, DUMP_PUBLISHER, STATUS_PUBLISHER, STEREO, TEA5767

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tea5767"]

ENTITY_SCHEMA = binary_sensor.binary_sensor_schema(icon="mdi:surround-sound").extend(
    {
        cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
    }
)
CONFIG_SCHEMA = cv.typed_schema(
    {"mono": ENTITY_SCHEMA, "stereo": ENTITY_SCHEMA}, lower=True
)


async def to_code(config: ConfigType) -> None:
    var = await binary_sensor.new_binary_sensor(config)
    hub = await cg.get_variable(config[CONF_TEA5767_ID])
    entity = automation.parent_ref(var)
    value = STEREO if config[CONF_TYPE] == "stereo" else cg.RawExpression(f"!{STEREO}")
    STATUS_PUBLISHER.add(hub, entity.publish_state(value))
    kind = "Stereo" if config[CONF_TYPE] == "stereo" else "Mono"
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_BINARY_SENSOR("", "TEA5767 Reception", {entity});')
    )
    DUMP_PUBLISHER.add(hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Type: {kind}");'))
