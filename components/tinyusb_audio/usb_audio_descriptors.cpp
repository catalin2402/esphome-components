#include "tinyusb_audio.h"
#include "esphome/core/defines.h"

#ifdef USE_TINYUSB_AUDIO_KEYBOARD
#include "esphome/components/tinyusb_keyboard/keyboard.h"
#endif

#include <algorithm>
#include <array>

#include "tinyusb.h"

namespace esphome::tinyusb_audio {

static constexpr uint8_t CLOCK_ID = 4;
static constexpr uint8_t INPUT_TERMINAL_ID = 1;
static constexpr uint8_t OUTPUT_TERMINAL_ID = 2;
static constexpr uint8_t PLAYBACK_FEATURE_ID = 3;
static constexpr uint32_t PLAYBACK_FEATURE_CONTROLS =
    (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS) | (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS);
static constexpr uint16_t PLAYBACK_VOLUME_STEP = 128;  // 0.5 dB
static constexpr uint8_t PLAYBACK_OUT = 0x01;
static constexpr uint8_t PLAYBACK_FEEDBACK = 0x82;
static constexpr uint8_t CAPTURE_IN = 0x83;
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
static constexpr uint8_t KEYBOARD_IN = 0x84;
static constexpr uint8_t KEYBOARD_PACKET_BYTES = sizeof(hid_keyboard_report_t) + 1;
static_assert(KEYBOARD_PACKET_BYTES <= CFG_TUD_HID_EP_BUFSIZE);
static constexpr uint8_t KEYBOARD_INTERFACES = 1;
static constexpr uint16_t KEYBOARD_DESCRIPTOR_LENGTH = TUD_HID_DESC_LEN;
static constexpr uint8_t CONFIG_ATTRIBUTES = TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP;
#else
static constexpr uint8_t KEYBOARD_INTERFACES = 0;
static constexpr uint16_t KEYBOARD_DESCRIPTOR_LENGTH = 0;
static constexpr uint8_t CONFIG_ATTRIBUTES = 0;
#endif
static constexpr uint32_t STEREO = AUDIO_CHANNEL_CONFIG_FRONT_LEFT | AUDIO_CHANNEL_CONFIG_FRONT_RIGHT;
#if TINYUSB_AUDIO_CAPTURE
static constexpr uint32_t CAPTURE_CHANNEL_CONFIG =
    TINYUSB_AUDIO_CAPTURE_CHANNELS == 2 ? STEREO : AUDIO_CHANNEL_CONFIG_FRONT_CENTER;
#endif
static constexpr uint8_t ISO_ASYNC_DATA =
    uint8_t{TUSB_XFER_ISOCHRONOUS} | uint8_t{TUSB_ISO_EP_ATT_ASYNCHRONOUS} | uint8_t{TUSB_ISO_EP_ATT_DATA};
static constexpr uint16_t CONFIG_LENGTH = TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO_FUNC_1_DESC_LEN +
                                          KEYBOARD_DESCRIPTOR_LENGTH
#if TINYUSB_AUDIO_CAPTURE
                                          + CFG_TUD_AUDIO_FUNC_2_DESC_LEN
#endif
    ;

// Each Windows audio function needs its own IAD and a single clock source.
// clang-format off
#define AUDIO_CONTROL(itf, category, input_type, channels, channel_config, feature_bytes) \
  TUD_AUDIO_DESC_IAD(itf, 2, 0), \
  TUD_AUDIO_DESC_STD_AC(itf, 0, 0), \
  TUD_AUDIO_DESC_CS_AC(0x0200, category, TINYUSB_AUDIO_TERMINAL_BYTES + feature_bytes, 0), \
  TUD_AUDIO_DESC_CLK_SRC(CLOCK_ID, AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK, 0x05, 0, 0), \
  TUD_AUDIO_DESC_INPUT_TERM(INPUT_TERMINAL_ID, input_type, 0, CLOCK_ID, channels, channel_config, 0, 0, 0)

#define AUDIO_STREAM(itf, terminal, channels, channel_config, bits, endpoint, packet_bytes, endpoints, interval) \
  TUD_AUDIO_DESC_STD_AS_INT(itf, 0, 0, 0), \
  TUD_AUDIO_DESC_STD_AS_INT(itf, 1, endpoints, 0), \
  TUD_AUDIO_DESC_CS_AS_INT(terminal, 0, AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM, channels, channel_config, 0), \
  TUD_AUDIO_DESC_TYPE_I_FORMAT(bits / 8, bits), \
  TUD_AUDIO_DESC_STD_AS_ISO_EP(endpoint, ISO_ASYNC_DATA, packet_bytes, interval), \
  TUD_AUDIO_DESC_CS_AS_ISO_EP(AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, 0, 0, 0)

#define PLAYBACK_DESCRIPTORS(interval) \
  AUDIO_CONTROL(PLAYBACK_INTERFACE - 1, AUDIO_FUNC_DESKTOP_SPEAKER, AUDIO_TERM_TYPE_USB_STREAMING, \
                2, STEREO, TINYUSB_AUDIO_PLAYBACK_FEATURE_BYTES), \
  TUD_AUDIO20_DESC_FEATURE_UNIT(PLAYBACK_FEATURE_ID, INPUT_TERMINAL_ID, 0, PLAYBACK_FEATURE_CONTROLS, 0, 0), \
  TUD_AUDIO_DESC_OUTPUT_TERM(OUTPUT_TERMINAL_ID, AUDIO_TERM_TYPE_OUT_GENERIC_SPEAKER, 0, PLAYBACK_FEATURE_ID, \
                             CLOCK_ID, 0, 0), \
  AUDIO_STREAM(PLAYBACK_INTERFACE, INPUT_TERMINAL_ID, 2, STEREO, TINYUSB_AUDIO_PLAYBACK_BITS, PLAYBACK_OUT, \
               TINYUSB_AUDIO_PLAYBACK_PACKET, 2, interval), \
  TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(PLAYBACK_FEEDBACK, 4, interval),

#if TINYUSB_AUDIO_CAPTURE
#define CAPTURE_DESCRIPTORS(interval) \
  AUDIO_CONTROL(CAPTURE_INTERFACE - 1, AUDIO_FUNC_MICROPHONE, AUDIO_TERM_TYPE_IN_GENERIC_MIC, \
                TINYUSB_AUDIO_CAPTURE_CHANNELS, CAPTURE_CHANNEL_CONFIG, 0), \
  TUD_AUDIO_DESC_OUTPUT_TERM(OUTPUT_TERMINAL_ID, AUDIO_TERM_TYPE_USB_STREAMING, 0, INPUT_TERMINAL_ID, CLOCK_ID, 0, 0), \
  AUDIO_STREAM(CAPTURE_INTERFACE, OUTPUT_TERMINAL_ID, TINYUSB_AUDIO_CAPTURE_CHANNELS, CAPTURE_CHANNEL_CONFIG, \
               TINYUSB_AUDIO_CAPTURE_BITS, CAPTURE_IN, \
               TINYUSB_AUDIO_CAPTURE_PACKET, 1, interval),
#else
#define CAPTURE_DESCRIPTORS(interval)
#endif

#ifdef USE_TINYUSB_AUDIO_KEYBOARD
#define KEYBOARD_DESCRIPTOR(interval) \
  TUD_HID_DESCRIPTOR(2 * CFG_TUD_AUDIO, 0, HID_ITF_PROTOCOL_NONE, sizeof(tinyusb_keyboard::HID_REPORT_DESCRIPTOR), \
                     KEYBOARD_IN, KEYBOARD_PACKET_BYTES, interval),
#else
#define KEYBOARD_DESCRIPTOR(interval)
#endif

#define AUDIO_CONFIGURATION(interval, keyboard_interval) \
  TUD_CONFIG_DESCRIPTOR(1, 2 * CFG_TUD_AUDIO + KEYBOARD_INTERFACES, 0, CONFIG_LENGTH, CONFIG_ATTRIBUTES, 100), \
  PLAYBACK_DESCRIPTORS(interval) \
  CAPTURE_DESCRIPTORS(interval) \
  KEYBOARD_DESCRIPTOR(keyboard_interval)
// clang-format on

// One transfer per millisecond at either speed. Windows uses 16.16 feedback at full speed as well.
const uint8_t CONFIGURATION_DESCRIPTOR[] = {AUDIO_CONFIGURATION(1, 10)};
static_assert(sizeof(CONFIGURATION_DESCRIPTOR) == CONFIG_LENGTH);

#if CONFIG_IDF_TARGET_ESP32P4 || CONFIG_IDF_TARGET_ESP32S31
static constexpr uint8_t HIGH_SPEED_CONFIGURATION_DESCRIPTOR[] = {AUDIO_CONFIGURATION(4, 7)};
static_assert(sizeof(HIGH_SPEED_CONFIGURATION_DESCRIPTOR) == CONFIG_LENGTH);
static constexpr tusb_desc_device_qualifier_t DEVICE_QUALIFIER{
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 1,
    .bReserved = 0,
};
#endif

static tusb_desc_device_t device_descriptor;
static tinyusb_event_cb_t upstream_event_callback;

static void on_usb_event(tinyusb_event_t *event, void *arg) {
  if (global_tinyusb_audio != nullptr) {
    if (event->id == TINYUSB_EVENT_SUSPENDED) {
      tud_audio_n_clear_ep_out_ff(0);
#if TINYUSB_AUDIO_CAPTURE
      tud_audio_n_clear_ep_in_ff(1);
#endif
      global_tinyusb_audio->on_usb_event(true, false);
    } else if (event->id == TINYUSB_EVENT_RESUMED) {
      global_tinyusb_audio->on_usb_event(false, false);
    } else if (event->id == TINYUSB_EVENT_DETACHED || event->id == TINYUSB_EVENT_ATTACHED) {
      global_tinyusb_audio->on_usb_event(false, true);
    }
  }
  if (upstream_event_callback != nullptr)
    upstream_event_callback(event, arg);
}

static bool set_interface(const tusb_control_request_t *request, bool active) {
  const auto interface = tu_u16_low(request->wIndex);
  if (global_tinyusb_audio != nullptr)
    global_tinyusb_audio->set_interface(interface, active);
  return true;
}

static constexpr uint32_t NOMINAL_FEEDBACK = (uint64_t{TINYUSB_AUDIO_PLAYBACK_RATE} << 16) / 1000;
static constexpr uint32_t MAX_FEEDBACK = ((TINYUSB_AUDIO_PLAYBACK_RATE / 1000) + 1) << 16;
static int32_t filtered_fill_error;

static void set_feedback(uint8_t function, uint32_t samples_per_ms) {
  // High-speed feedback is reported per microframe, even with a 1 ms transfer interval.
  tud_audio_n_fb_set(function, tud_speed_get() == TUSB_SPEED_HIGH ? samples_per_ms / 8 : samples_per_ms);
}

extern "C" esp_err_t __real_tinyusb_driver_install(const tinyusb_config_t *config);

extern "C" esp_err_t __wrap_tinyusb_driver_install(const tinyusb_config_t *config) {
  if (config->descriptor.full_speed_config != CONFIGURATION_DESCRIPTOR
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
      && config->descriptor.full_speed_config != tinyusb_keyboard::CONFIGURATION_DESCRIPTOR
#endif
  )
    return __real_tinyusb_driver_install(config);

  // Preserve the configured USB identity and task settings.
  tinyusb_config_t updated = *config;
  // Either class may supply the initial descriptor during code generation.
  updated.descriptor.full_speed_config = CONFIGURATION_DESCRIPTOR;
#if CONFIG_IDF_TARGET_ESP32P4
  updated.port = TINYUSB_PORT_HIGH_SPEED_0;
#endif
#if CONFIG_IDF_TARGET_ESP32P4 || CONFIG_IDF_TARGET_ESP32S31
  updated.descriptor.high_speed_config = HIGH_SPEED_CONFIGURATION_DESCRIPTOR;
  updated.descriptor.qualifier = &DEVICE_QUALIFIER;
#endif
  device_descriptor = *config->descriptor.device;
  device_descriptor.bDeviceClass = TUSB_CLASS_MISC;
  device_descriptor.bDeviceSubClass = MISC_SUBCLASS_COMMON;
  device_descriptor.bDeviceProtocol = MISC_PROTOCOL_IAD;
  updated.descriptor.device = &device_descriptor;
  upstream_event_callback = config->event_cb;
  updated.event_cb = on_usb_event;
  return __real_tinyusb_driver_install(&updated);
}

static bool is_playback_feature_request(const tusb_control_request_t *request) {
  return tu_u16_high(request->wIndex) == PLAYBACK_FEATURE_ID && tu_u16_low(request->wIndex) == PLAYBACK_INTERFACE - 1 &&
         tu_u16_low(request->wValue) == 0;
}

static bool get_playback_feature(uint8_t rhport, const tusb_control_request_t *request) {
  if (global_tinyusb_audio == nullptr)
    return false;
  const auto selector = tu_u16_high(request->wValue);
  if (selector == AUDIO20_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR) {
    uint8_t muted = global_tinyusb_audio->host_muted();
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &muted, sizeof(muted));
  }
  if (selector != AUDIO20_FU_CTRL_VOLUME)
    return false;
  if (request->bRequest == AUDIO_CS_REQ_CUR) {
    uint16_t volume = tu_htole16(static_cast<uint16_t>(global_tinyusb_audio->host_volume()));
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &volume, sizeof(volume));
  }
  if (request->bRequest == AUDIO_CS_REQ_RANGE) {
    uint16_t range[] = {tu_htole16(1), tu_htole16(static_cast<uint16_t>(PLAYBACK_VOLUME_MIN)), 0,
                        tu_htole16(PLAYBACK_VOLUME_STEP)};
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, range, sizeof(range));
  }
  return false;
}

extern "C" bool tud_audio_set_req_entity_cb(uint8_t, const tusb_control_request_t *request, uint8_t *buffer) {
  if (global_tinyusb_audio == nullptr || !is_playback_feature_request(request) || request->bRequest != AUDIO_CS_REQ_CUR)
    return false;
  const auto selector = tu_u16_high(request->wValue);
  if (selector == AUDIO20_FU_CTRL_MUTE && request->wLength == 1) {
    global_tinyusb_audio->set_host_muted(buffer[0] != 0);
    return true;
  }
  if (selector == AUDIO20_FU_CTRL_VOLUME && request->wLength == 2) {
    const auto volume = static_cast<int16_t>(tu_u16(buffer[1], buffer[0]));
    global_tinyusb_audio->set_host_volume(std::clamp<int16_t>(volume, PLAYBACK_VOLUME_MIN, 0));
    return true;
  }
  return false;
}

extern "C" bool tud_audio_get_req_entity_cb(uint8_t rhport, const tusb_control_request_t *request) {
  if (is_playback_feature_request(request))
    return get_playback_feature(rhport, request);
  if (tu_u16_high(request->wIndex) != CLOCK_ID || tu_u16_low(request->wValue) != 0)
    return false;

  uint32_t rate;
  switch (tu_u16_low(request->wIndex)) {
    case PLAYBACK_INTERFACE - 1:
      rate = TINYUSB_AUDIO_PLAYBACK_RATE;
      break;
#if TINYUSB_AUDIO_CAPTURE
    case CAPTURE_INTERFACE - 1:
      rate = TINYUSB_AUDIO_CAPTURE_RATE;
      break;
#endif
    default:
      return false;
  }

  const auto control = tu_u16_high(request->wValue);
  if (control == AUDIO_CS_CTRL_CLK_VALID && request->bRequest == AUDIO_CS_REQ_CUR) {
    uint8_t valid = 1;
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &valid, sizeof(valid));
  }
  if (control != AUDIO_CS_CTRL_SAM_FREQ)
    return false;
  if (request->bRequest == AUDIO_CS_REQ_CUR) {
    uint32_t current = tu_htole32(rate);
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &current, sizeof(current));
  }
  if (request->bRequest == AUDIO_CS_REQ_RANGE) {
    audio_control_range_4_n_t(1) range{};
    range.wNumSubRanges = tu_htole16(1);
    range.subrange[0].bMin = tu_htole32(rate);
    range.subrange[0].bMax = tu_htole32(rate);
    return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &range, sizeof(range));
  }
  return false;
}

extern "C" void tud_audio_feedback_params_cb(uint8_t, uint8_t, audio_feedback_params_t *params) {
  params->method = AUDIO_FEEDBACK_METHOD_DISABLED;  // The receive callback computes feedback.
  params->sample_freq = TINYUSB_AUDIO_PLAYBACK_RATE;
}

extern "C" bool tud_audio_set_itf_cb(uint8_t, const tusb_control_request_t *request) {
  const bool active = tu_u16_low(request->wValue) == 1;
  if (active && tu_u16_low(request->wIndex) == PLAYBACK_INTERFACE) {
    filtered_fill_error = 0;
    set_feedback(0, NOMINAL_FEEDBACK);
  }
  return set_interface(request, active);
}

extern "C" bool tud_audio_set_itf_close_ep_cb(uint8_t, const tusb_control_request_t *request) {
  return set_interface(request, false);
}

extern "C" bool tud_audio_rx_done_isr(uint8_t, uint16_t received, uint8_t function, uint8_t, uint8_t) {
  std::array<uint8_t, TINYUSB_AUDIO_PLAYBACK_PACKET> packet;
  const size_t length = tud_audio_n_read(function, packet.data(), std::min<size_t>(received, packet.size()));
  if (global_tinyusb_audio == nullptr || !global_tinyusb_audio->is_playback_active())
    return true;

  global_tinyusb_audio->receive_playback(std::span(packet).first(length));
  // Smooth queue occupancy before adjusting the host's send rate. The receive
  // queue tracks the speaker clock and absorbs jitter from the firmware loop.
  const int32_t buffered = global_tinyusb_audio->playback_buffered_frames();
  constexpr int32_t TARGET_FRAMES = TINYUSB_AUDIO_PLAYBACK_RATE * 20 / 1000;
  constexpr int32_t MAX_CORRECTION = 8192;  // 1/8 frame per USB millisecond.
  filtered_fill_error += buffered - TARGET_FRAMES - filtered_fill_error / 128;
  const int32_t correction = std::clamp(filtered_fill_error / 8, -MAX_CORRECTION, MAX_CORRECTION);
  const auto feedback = static_cast<uint32_t>(static_cast<int32_t>(NOMINAL_FEEDBACK) - correction);
  set_feedback(function, std::min(feedback, MAX_FEEDBACK));
  return true;
}

#if TINYUSB_AUDIO_CAPTURE
extern "C" bool tud_audio_tx_done_isr(uint8_t, uint16_t, uint8_t, uint8_t, uint8_t) {
  if (global_tinyusb_audio != nullptr)
    global_tinyusb_audio->fill_capture_packet();
  return true;
}
#endif

}  // namespace esphome::tinyusb_audio
