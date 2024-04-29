import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_LENGTH,
)
from esphome.core.entity_helpers import inherit_property_from
from esphome.components import display_7segment_base
from esphome.components.display_menu_base import (
    DISPLAY_MENU_BASE_SCHEMA,
    DisplayMenuComponent,
    display_menu_to_code,
)

CODEOWNERS = ["@asergunov"]

AUTO_LOAD = ["display_menu_base", "display_7segment_base"]

lcd_menu_7segment_ns = cg.esphome_ns.namespace("lcd_menu_7segment")

CONF_DISPLAY_ID = "display_id"

CONF_MARK_EDITING = "mark_editing"
CONF_MARK_SUBMENU = "mark_submenu"
CONF_MARK_BACK = "mark_back"

MINIMUM_COLUMNS = 12

LCD7SegmentMenuComponent = lcd_menu_7segment_ns.class_(
    "LCD7SegmentMenuComponent", DisplayMenuComponent
)

MULTI_CONF = True


def validate_lcd_dimensions(config):
    return config


CONFIG_SCHEMA = DISPLAY_MENU_BASE_SCHEMA.extend(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LCD7SegmentMenuComponent),
            cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(display_7segment_base.Display),
            cv.Optional(CONF_MARK_EDITING, default=0x2A): cv.uint8_t,
            cv.Optional(CONF_MARK_SUBMENU, default=0x7E): cv.uint8_t,
            cv.Optional(CONF_MARK_BACK, default=0x5E): cv.uint8_t,
        }
    )
)

FINAL_VALIDATE_SCHEMA = cv.All(
    inherit_property_from(CONF_LENGTH, CONF_DISPLAY_ID),
    validate_lcd_dimensions,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    disp = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_display(disp))
    cg.add(var.set_length(config[CONF_LENGTH]))
    await display_menu_to_code(var, config)
    cg.add(var.set_mark_editing(config[CONF_MARK_EDITING]))
    cg.add(var.set_mark_submenu(config[CONF_MARK_SUBMENU]))
    cg.add(var.set_mark_back(config[CONF_MARK_BACK]))
