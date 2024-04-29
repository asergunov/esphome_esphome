import esphome.codegen as cg

display_7segment_base_ns = cg.esphome_ns.namespace("display_7segment_base")
Display = display_7segment_base_ns.class_("Display", cg.PollingComponent)
DisplayRef = Display.operator("ref")
