import esphome.codegen as cg
from esphome.components import media_player
from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import ID

from .. import Vs10xxAudioComponent, vs10xx_ns

CODEOWNERS = ["@asergunov"]
DEPENDENCIES = ["vs10xx", "http_request"]


CONF_VS10XX_AUDIO_ID = "vs10xx_audio_id"

Vs10xxMediaPlayer = vs10xx_ns.class_(
    "Vs10xxMediaPlayer", cg.Component, media_player.MediaPlayer
)

CONFIG_SCHEMA = cv.All(
    media_player.MEDIA_PLAYER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Vs10xxMediaPlayer),
            cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
            cv.GenerateID(CONF_VS10XX_AUDIO_ID): cv.use_id(Vs10xxAudioComponent),
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
    await cg.register_parented(var, config[CONF_VS10XX_AUDIO_ID])

    if http_request := config[CONF_HTTP_REQUEST_ID]:
        if isinstance(http_request, ID):
            http_request = await cg.get_variable(http_request)
        cg.add(var.set_http_request(http_request))
