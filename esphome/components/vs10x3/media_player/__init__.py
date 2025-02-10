import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE

from .. import Vs10x3AudioComponent, vs10x3_ns

CODEOWNERS = ["@asergunov"]
DEPENDENCIES = ["vs10x3", "network"]


CONF_VS10X3_AUDIO_ID = "vs10x3_audio_id"

Vs10x3MediaPlayer = vs10x3_ns.class_(
    "Vs10x3MediaPlayer", cg.Component, media_player.MediaPlayer
)

CONFIG_SCHEMA = cv.All(
    media_player.MEDIA_PLAYER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Vs10x3MediaPlayer),
            cv.GenerateID(CONF_VS10X3_AUDIO_ID): cv.use_id(Vs10x3AudioComponent),
        },
    ),
    cv.only_with_arduino,
    cv.only_on_esp32,
    cv.require_framework_version(
        esp8266_arduino=cv.Version(2, 5, 1),
        esp32_arduino=cv.Version(0, 0, 0),
        esp_idf=cv.Version(0, 0, 0),
        rp2040_arduino=cv.Version(0, 0, 0),
    ),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_player.register_media_player(var, config)
    await cg.register_parented(var, config[CONF_VS10X3_AUDIO_ID])
