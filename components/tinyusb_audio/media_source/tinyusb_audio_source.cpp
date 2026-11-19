#include "tinyusb_audio_source.h"

#include "esphome/core/log.h"

namespace esphome::tinyusb_audio {

static constexpr char USB_URI[] = "usb://current";
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
static constexpr uint32_t MEDIA_KEY_RELEASE_TIMEOUT = 0;
#endif
// Run above the ESPHome loop while leaving TinyUSB and the I²S driver ahead of the producer.
static constexpr UBaseType_t PLAYBACK_TASK_PRIORITY = 4;
static constexpr uint32_t PLAYBACK_TASK_STACK_SIZE = 4096;

void TinyUSBAudioSource::setup() {
  if (!this->playback_task_.create([](void *arg) { static_cast<TinyUSBAudioSource *>(arg)->playback_task_run_(); },
                                   "usb_audio_play", PLAYBACK_TASK_STACK_SIZE, this, PLAYBACK_TASK_PRIORITY, false)) {
    this->mark_failed(LOG_STR("Could not create USB audio playback task"));
  }
}

bool TinyUSBAudioSource::can_handle(const std::string &uri) const { return uri == USB_URI; }

bool TinyUSBAudioSource::play_uri(const std::string &uri) {
  if (!this->can_handle(uri) || !this->has_listener() || !this->parent_->is_playback_active() || this->is_failed())
    return false;
  this->generation_ = this->parent_->playback_generation();
  this->selected_generation_.store(this->generation_);
  this->set_state_(media_source::MediaSourceState::PLAYING);
  this->selection_.fetch_add(this->is_selected_() ? 2 : 1);
  xTaskNotifyGive(this->playback_task_.get_handle());
  return true;
}

void TinyUSBAudioSource::handle_command(media_source::MediaSourceCommand command) {
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
  switch (command) {
    case media_source::MediaSourceCommand::PLAY:
    case media_source::MediaSourceCommand::PAUSE:
      this->send_media_key_(0xCD);  // Play/pause
      break;
    case media_source::MediaSourceCommand::NEXT:
      this->send_media_key_(0xB5);  // Next track
      break;
    case media_source::MediaSourceCommand::PREVIOUS:
      this->send_media_key_(0xB6);  // Previous track
      break;
    default:
      break;
  }
#endif
  switch (command) {
    case media_source::MediaSourceCommand::STOP:
      this->stop_();
      this->set_state_(media_source::MediaSourceState::IDLE);
      break;
    case media_source::MediaSourceCommand::PAUSE:
      if (this->parent_->is_playback_keep_alive() && this->is_selected_())
        break;
      this->stop_();
      this->set_state_(media_source::MediaSourceState::PAUSED);
      break;
    case media_source::MediaSourceCommand::PLAY:
      this->play_uri(USB_URI);
      break;
    default:
      break;
  }
}

#ifdef USE_TINYUSB_AUDIO_KEYBOARD
void TinyUSBAudioSource::send_media_key_(uint16_t usage) {
  this->keyboard_->press_media(usage);
  this->set_timeout(MEDIA_KEY_RELEASE_TIMEOUT, 50, [this]() { this->keyboard_->release_media(); });
}
#endif

void TinyUSBAudioSource::stop_() {
  if (this->is_selected_())
    this->selection_.fetch_add(1);
  this->parent_->clear_playback();
}

void TinyUSBAudioSource::loop() {
  if (this->parent_->take_host_volume_change())
    this->request_volume_(1.0f - static_cast<float>(this->parent_->host_volume()) / PLAYBACK_VOLUME_MIN);
  if (this->parent_->take_host_mute_change())
    this->request_mute_(this->parent_->host_muted());

  const auto generation = this->parent_->playback_generation();
  if (generation != this->generation_) {
    this->generation_ = generation;
    this->auto_start_ready_ = false;
    if (this->is_selected_()) {
      this->stop_();
      this->set_state_(media_source::MediaSourceState::IDLE);
    }
  }

  const bool ready = this->parent_->should_auto_start_playback();
  if (ready != this->auto_start_ready_) {
    this->auto_start_ready_ = ready;
    if (ready && !this->is_selected_()) {
      this->request_play_uri_(USB_URI);
    } else if (!ready && this->is_selected_()) {
      this->stop_();
      this->set_state_(media_source::MediaSourceState::IDLE);
    }
  }
}

void TinyUSBAudioSource::playback_task_run_() {
  const audio::AudioStreamInfo format(TINYUSB_AUDIO_PLAYBACK_BITS, 2, TINYUSB_AUDIO_PLAYBACK_RATE);
  uint32_t last_selection = 0;
  size_t bytes = 0;
  size_t offset = 0;

  while (true) {
    const uint32_t selection = this->selection_.load();
    if (selection != last_selection) {
      last_selection = selection;
      bytes = offset = 0;
    }
    if ((selection & 1) == 0) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }

    const uint32_t generation = this->selected_generation_.load();
    if (generation != this->parent_->playback_generation()) {
      vTaskDelay(1);
      continue;
    }
    if (offset == bytes) {
      bytes = this->parent_->read_playback(this->pending_);
      offset = 0;
    }
    if (bytes == 0 || selection != this->selection_.load() || generation != this->parent_->playback_generation()) {
      vTaskDelay(1);
      continue;
    }

    const size_t accepted = this->write_output(this->pending_.data() + offset, bytes - offset, 2, format);
    offset += accepted;
    if (accepted == 0)
      vTaskDelay(1);
  }
}

}  // namespace esphome::tinyusb_audio
