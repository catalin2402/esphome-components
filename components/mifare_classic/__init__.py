import esphome.codegen as cg

DOMAIN = "mifare_classic"

CODEOWNERS = ["@catalin2402"]
AUTO_LOAD = ["nfc"]
mifare_classic_ns = cg.esphome_ns.namespace(DOMAIN)
MifareClassicReader = mifare_classic_ns.class_("MifareClassicReader")
