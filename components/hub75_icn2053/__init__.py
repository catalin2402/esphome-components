import esphome.codegen as cg
from esphome.components import display

CODEOWNERS = ["@catalin2402"]

hub75_icn2053_ns = cg.esphome_ns.namespace("hub75_icn2053")
HUB75_ICN2053 = hub75_icn2053_ns.class_(
    "HUB75_ICN2053",
    display.DisplayBuffer,
)
HUB75_ICN2053_ID = "hub75_icn2053_id"
