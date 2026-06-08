import esphome.config_validation as cv

CONF_RECEIVER_ID = "receiver_id"
CONF_REMOTE_ID = "remote_id"
CONF_REMOTE_BUTTON = "remote_button"
CONF_RESET_TIME = "reset_time"


def validate_remote_id(value):
    value = cv.string(value).upper().replace("0X", "")
    if len(value) == 0 or len(value) > 6:
        raise cv.Invalid(
            "remote_id must be a hex string with 1 to 6 characters, for example '098B91'"
        )
    try:
        int(value, 16)
    except ValueError:
        raise cv.Invalid("remote_id must be hexadecimal, for example '098B91'")
    return value
