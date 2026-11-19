from esphome import automation
import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, STATE_CLASS_MEASUREMENT, UNIT_HERTZ
from esphome.types import ConfigType

from .. import (
    CONF_TEA5767_ID,
    DUMP_PUBLISHER,
    FREQUENCY,
    LEVEL,
    STATUS_PUBLISHER,
    TEA5767,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tea5767"]


def entity_schema(**kwargs):
    return sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        **kwargs,
    ).extend(
        {
            cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "frequency": entity_schema(
            unit_of_measurement=UNIT_HERTZ, icon="mdi:radio-tower"
        ),
        "signal_level": entity_schema(icon="mdi:signal"),
    },
    lower=True,
)


async def to_code(config: ConfigType) -> None:
    var = await sensor.new_sensor(config)
    hub = await cg.get_variable(config[CONF_TEA5767_ID])
    entity = automation.parent_ref(var)
    value = FREQUENCY if config[CONF_TYPE] == "frequency" else LEVEL
    STATUS_PUBLISHER.add(hub, entity.publish_state(value))
    kind = "Frequency" if config[CONF_TYPE] == "frequency" else "Signal level"
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SENSOR("", "TEA5767 Sensor", {entity});')
    )
    DUMP_PUBLISHER.add(hub, cg.RawStatement(f'ESP_LOGCONFIG(TAG, "  Type: {kind}");'))
