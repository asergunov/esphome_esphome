import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_SEARCH_STOP_LEVEL = "search_stop_level"
CONF_CLOCK_FREQUENCY = "clock_frequency"
CONF_PLL_EXTERNAL_REF_FREQ = "pll_external_reference_frequency"
CONF_DTC = "dtc"
CONF_BAND = "band"

DEPENDENCIES = ["i2c"]

tea5767_ns = cg.esphome_ns.namespace("tea5767")
Tea5767Component = tea5767_ns.class_("Tea5767Component", cg.Component, i2c.I2CDevice)
SSL = Tea5767Component.enum("SSL")
CF = Tea5767Component.enum("CF")
DTC = Tea5767Component.enum("DTC")
Band = Tea5767Component.enum("Band")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tea5767Component),
            cv.Optional(CONF_SEARCH_STOP_LEVEL, default="MID"): cv.enum(
                {
                    "LOW": SSL.SSL_LOW,
                    "MID": SSL.SSL_MID,
                    "HIGH": SSL.SSL_HIGH,
                    "5": SSL.SSL_LOW,
                    "7": SSL.SSL_MID,
                    "10": SSL.SSL_HIGH,
                },
                upper=True,
            ),
            cv.Optional(CONF_CLOCK_FREQUENCY, default="32.768kHz"): cv.All(
                cv.frequency,
                cv.one_of(32768, 13e6),
                cv.enum(
                    {
                        32768: CF.CF_32768Hz,
                        13e6: CF.CF_13MHz,
                        # 6.5e6: CF.CF_6500kHz,
                    }
                ),
            ),
            cv.Optional(CONF_PLL_EXTERNAL_REF_FREQ): cv.All(
                cv.frequency,
                cv.one_of(6.5e6),
            ),
            cv.Optional(CONF_DTC, default="50us"): cv.All(
                # cv.time_period_microseconds,
                cv.enum(
                    {
                        "50us": DTC.DTC_50us,
                        "75us": DTC.DTC_75us,
                    }
                ),
            ),
            cv.Optional(CONF_BAND, default="Europe"): cv.enum(
                {
                    "Japan": Band.Japanese,
                    "Europe": Band.Europe,
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0b1100000))
)


def validate_clock(config):
    if CONF_PLL_EXTERNAL_REF_FREQ in config and config[CONF_CLOCK_FREQUENCY] == 32768:
        raise cv.Invalid("Only 13MHz crystall can be used with external PLL reference")
    return config


FINAL_VALIDATE_SCHEMA = cv.All(
    i2c.final_validate_device_schema("tea5767", max_frequency="400khz"), validate_clock
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    if search_stop_level := config[CONF_SEARCH_STOP_LEVEL]:
        cg.add(var.set_search_stop_level(search_stop_level))
    if clock_frequency := config[CONF_CLOCK_FREQUENCY]:
        cg.add(var.set_clock_frequency(clock_frequency))
    if CONF_PLL_EXTERNAL_REF_FREQ in config:
        cg.add(var.set_external_pll())
    if dtc := config[CONF_DTC]:
        cg.add(var.set_dtc(dtc))
    if band := config[CONF_BAND]:
        cg.add(var.set_band(band))
