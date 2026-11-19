from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
    UNIT_HOUR,
)
from esphome.types import ConfigType

from .. import (
    CONF_BAMBU_LAB_TAGS_ID,
    DUMP_PUBLISHER,
    SPOOL,
    TAG_PUBLISHER,
    BambuLabTags,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["bambu_lab_tags"]

BASE_SCHEMA = cv.Schema(
    {cv.GenerateID(CONF_BAMBU_LAB_TAGS_ID): cv.use_id(BambuLabTags)}
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "WEIGHT": BASE_SCHEMA.extend(
            sensor.sensor_schema(unit_of_measurement="g", accuracy_decimals=0)
        ),
        "DRYING_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "DRYING_TIME": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                accuracy_decimals=0,
                unit_of_measurement=UNIT_HOUR,
                device_class=DEVICE_CLASS_DURATION,
            )
        ),
        "BED_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "MIN_NOZZLE_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
        "MAX_NOZZLE_TEMPERATURE": BASE_SCHEMA.extend(
            sensor.sensor_schema(
                accuracy_decimals=0,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
            )
        ),
    },
    upper=True,
)


async def to_code(config: ConfigType) -> None:
    var = await sensor.new_sensor(config)
    hub = await cg.get_variable(config[CONF_BAMBU_LAB_TAGS_ID])
    entity = parent_ref(var)
    value = getattr(SPOOL, config[CONF_TYPE].lower())
    TAG_PUBLISHER.add(hub, entity.publish_state(value))
    DUMP_PUBLISHER.add(
        hub, cg.RawStatement(f'LOG_SENSOR("", "Bambu Lab Tags", {entity});')
    )
