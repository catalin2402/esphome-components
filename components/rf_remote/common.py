import esphome.config_validation as cv

CONF_RECEIVER_ID = "receiver_id"
CONF_REMOTE_ID = "remote_id"
CONF_REMOTE_BUTTON = "remote_button"
CONF_RESET_TIME = "reset_time"


def validate_remote_id(value):
    if isinstance(value, int) and not isinstance(value, bool):
        if 0 <= value <= 0x3FFFF:
            return value
        raise cv.Invalid("remote_id must be between 0x00000 and 0x3FFFF")

    value = cv.string_strict(value).strip()
    if value.lower().startswith("0x"):
        value = value[2:]
    if not 1 <= len(value) <= 5:
        raise cv.Invalid(
            "remote_id must be a 1-5 character hex string, for example '18B91'"
        )
    try:
        result = int(value, 16)
    except ValueError:
        raise cv.Invalid("remote_id must be hexadecimal, for example '18B91'")
    if result > 0x3FFFF:
        raise cv.Invalid("remote_id must be between 0x00000 and 0x3FFFF")
    return result
