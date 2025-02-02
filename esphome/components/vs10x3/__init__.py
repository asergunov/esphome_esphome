from esphome import pins
import esphome.codegen as cg
from esphome.components import spi
import esphome.config_validation as cv
from esphome.const import (
    CONF_CS_PIN,
    CONF_ID,
    CONF_SPI_ID,
    KEY_CORE,
    KEY_TARGET_PLATFORM,
    PLATFORM_RP2040,
)
from esphome.core import CORE
from esphome.cpp_generator import MockObjClass
import esphome.final_validate as fv

CODEOWNERS = ["@setosha"]
CONF_DCS_PIN = "dcs_pin"
CONF_DREQ_PIN = "dreq_pin"
CONF_VS10X3_AUDIO_ID = "vs10x3_audio_id"

vs10x3_ns = cg.esphome_ns.namespace("vs10x3")
Vs10x3AudioComponent = vs10x3_ns.class_(
    "Vs10x3AudioComponent", cg.Component, spi.SPIDevice
)


def vs10x3_audio_component_schema(
    class_: MockObjClass,
):
    return cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(class_),
            cv.GenerateID(CONF_VS10X3_AUDIO_ID): cv.use_id(Vs10x3AudioComponent),
        }
    )


async def register_vs10x3_audio_component(var, config):
    await cg.register_parented(var, config[CONF_VS10X3_AUDIO_ID])


def validate_platform(obj):
    if CORE.data[KEY_CORE][KEY_TARGET_PLATFORM] == PLATFORM_RP2040:
        raise cv.Invalid(f"{PLATFORM_RP2040} is not supported")
    return obj


CONFIG_SCHEMA = cv.All(
    spi.spi_device_schema(cs_pin_required=True).extend(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(Vs10x3AudioComponent),
                cv.Required(CONF_DCS_PIN): pins.internal_gpio_output_pin_number,
                cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,
                cv.Required(CONF_DREQ_PIN): pins.internal_gpio_input_pin_number,
            }
        ),
    ),
    cv.only_with_arduino,
    validate_platform,
)


async def to_code(config):
    cg.add_library(
        "ESP_VS1053_Library",
        None,  # "1.1.4",
        "https://github.com/baldram/ESP_VS1053_Library.git",
        # "https://github.com/asergunov/ESP_VS1053_Library.git#spi-class-instance",
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_dcs_pin(config[CONF_DCS_PIN]))
    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    cg.add(var.set_dreq_pin(config[CONF_DREQ_PIN]))


def final_validate_schema():
    def validate_interface_index(spi_index):
        spi_interface = spi.get_spi_interface(spi_index)
        if spi_interface != "&SPI":
            raise cv.Invalid(
                f"Only default SPI Arduino supported. `{spi_interface}` requested."
            )
        return spi_index

    name = "vs10x3"
    return cv.Schema(
        {
            cv.Required(
                CONF_SPI_ID, msg="Only default SPI Arduino supported"
            ): fv.id_declaration_match_schema(
                {
                    cv.Required(spi.CONF_INTERFACE_INDEX): validate_interface_index,
                    cv.Required(
                        spi.CONF_MISO_PIN,
                        msg=f"Component {name} requires this spi bus to declare a miso_pin",
                    ): cv.valid,
                    cv.Required(
                        spi.CONF_MOSI_PIN,
                        msg=f"Component {name} requires this spi bus to declare a miso_pin",
                    ): cv.valid,
                }
            ),
        },
        extra=cv.ALLOW_EXTRA,
    )


FINAL_VALIDATE_SCHEMA = final_validate_schema()
