import esphome.codegen as cg

display_7segment_base_ns = cg.esphome_ns.namespace(
    "display_7segment_base", cg.PollingComponent
)
Display = display_7segment_base_ns.class_("Display")
DisplayRef = Display.operator("ref")
