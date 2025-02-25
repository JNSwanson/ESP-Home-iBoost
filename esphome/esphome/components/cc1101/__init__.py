import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# Define the namespace for the CC1101 component
cc1101_ns = cg.esphome_ns.namespace("cc1101")
CC1101 = cc1101_ns.class_("CC1101", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CC1101),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])  # No await needed
    await cg.register_component(var, config)
