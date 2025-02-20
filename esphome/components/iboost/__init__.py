import esphome.codegen as cg

CODEOWNERS = ["@JNSwanson"]

# Define the namespace
iboost_ns = cg.esphome_ns.namespace("iboost")

# Tell ESPHome to compile `iboost.cpp`
cg.add_library("iboost", None)

