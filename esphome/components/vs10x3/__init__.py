from esphome import pins
import esphome.codegen as cg
from esphome.components import spi
import esphome.config_validation as cv
from esphome.const import (
    CONF_CS_PIN,
    CONF_ID,
    CONF_RESET_PIN,
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
CONF_XTAL_FREQUENCY = "xtal_frequency"
CONF_CLOCK_MULTIPLIER = "clock_multiplier"
CONF_CLOCK_MULTIPLIER_ADD = "clock_multiplier_add"

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


def closest_spi_rate(rate):
    values_less = [k for k in spi.SPI_DATA_RATE_OPTIONS if k < rate]
    values_less.sort(reverse=True)
    return values_less[0]


CONFIG_SCHEMA = spi.spi_device_schema(
    cs_pin_required=True,
).extend(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Vs10x3AudioComponent),
            cv.Required(CONF_DCS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DREQ_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_XTAL_FREQUENCY, default="12.288MHz"): cv.All(
                cv.frequency, cv.Range(min=12e6, max=13e6)
            ),
            cv.Optional(CONF_CLOCK_MULTIPLIER, default=3.0): cv.one_of(
                *((x + 1) * 0.5 for x in range(0, 8)), float=True
            ),
            cv.Optional(CONF_CLOCK_MULTIPLIER_ADD, default=1.5): cv.one_of(
                *(x * 0.5 for x in range(0, 4)), float=True
            ),
        }
    ),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if dreq_pin := config.get(CONF_DREQ_PIN):
        cg.add(var.set_dreq_pin(await cg.gpio_pin_expression(dreq_pin)))
    if reset_pin := config.get(CONF_RESET_PIN):
        cg.add(var.set_reset_pin(await cg.gpio_pin_expression(reset_pin)))

    if xtal_frequency := config.get(CONF_XTAL_FREQUENCY):
        cg.add(var.set_xtal_frequency(xtal_frequency))

    cg.add(
        var.set_clock_multiplier(
            config[CONF_CLOCK_MULTIPLIER] * 2 - 1, config[CONF_CLOCK_MULTIPLIER_ADD] * 2
        )
    )

    await spi.register_spi_device(var, config)
    await spi.register_spi_device(
        var.get_data_device(),
        {
            k if k != CONF_DCS_PIN else CONF_CS_PIN: v
            for k, v in config.items()
            if k
            in {
                CONF_SPI_ID,
                CONF_DCS_PIN,
                spi.CONF_DATA_RATE,
                spi.CONF_SPI_MODE,
            }
        },
    )


def final_validate_schema():
    name = "vs10x3"
    return cv.Schema(
        {
            cv.Required(
                CONF_SPI_ID, msg="Only default SPI Arduino interface is supported"
            ): fv.id_declaration_match_schema(
                {
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
