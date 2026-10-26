idf_component_register()

# usb_device_uac 1.3.1 normally adds its audio-only descriptor source directly
# to TinyUSB. Replace only that source with the composite UAC + HID descriptor;
# the standard UAC implementation remains untouched.
idf_build_get_property(build_components BUILD_COMPONENTS)
if(tinyusb IN_LIST build_components)
    set(tinyusb_name tinyusb)
else()
    set(tinyusb_name espressif__tinyusb)
endif()

idf_component_get_property(tusb_lib ${tinyusb_name} COMPONENT_LIB)

foreach(source_property SOURCES INTERFACE_SOURCES)
    get_target_property(tusb_sources ${tusb_lib} ${source_property})
    if(tusb_sources)
        list(FILTER tusb_sources EXCLUDE REGEX "usb_descriptors\\.c")
        set_property(TARGET ${tusb_lib} PROPERTY ${source_property} "${tusb_sources}")
    endif()
endforeach()

target_compile_definitions(${tusb_lib} PUBLIC CFG_TUD_HID=1 CFG_TUD_HID_EP_BUFSIZE=16)
target_sources(${tusb_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/usb_audio_hid_descriptors.c")
