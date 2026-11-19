import esphome.codegen as cg

AUTO_LOAD = ["nfc"]
CODEOWNERS = ["@catalin2402"]
DOMAIN = "mifare_classic"

mifare_classic_ns = cg.esphome_ns.namespace(DOMAIN)
MifareClassicReader = mifare_classic_ns.class_("MifareClassicReader")
MifareClassicListener = mifare_classic_ns.class_("MifareClassicListener")
