#include "vs10xx_media_player.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx_media_player";

void Vs10xxMediaPlayer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Audio...");
  this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
  this->ring_buffer_ = make_unique<RingBuffer>();
}

media_player::MediaPlayerTraits Vs10xxMediaPlayer::get_traits() {
  media_player::MediaPlayerTraits traits;
  traits.set_supports_pause(true);
  return traits;
}

void Vs10xxMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  ESP_LOGV(TAG, "Control recieved");

  media_player::MediaPlayerState play_state = media_player::MEDIA_PLAYER_STATE_PLAYING;
  if (call.get_announcement().has_value()) {
    play_state = call.get_announcement().value() ? media_player::MEDIA_PLAYER_STATE_ANNOUNCING
                                                 : media_player::MEDIA_PLAYER_STATE_PLAYING;
  }

  const auto &media_url = call.get_media_url();
  if (media_url.has_value()) {
    ESP_LOGD(TAG, "Opening media_url: %s", media_url->c_str());
    this->high_freq_.start();
    this->http_container_ = http_request_->get(*media_url);
    if (this->http_container_) {
      this->state = play_state;
    } else {
      ESP_LOGW(TAG, "HttpRequest returned no container for url %s", media_url->c_str());
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
    }
  }

  if (play_state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING) {
    this->is_announcement_ = true;
  }

  if (call.get_volume().has_value()) {
    ESP_LOGD(TAG, "Has volume %f", call.get_volume().value());
    this->set_volume_(call.get_volume().value());
    this->unmute_();
  }
  if (call.get_command().has_value()) {
    ESP_LOGD(TAG, "Has command %d", call.get_command().value());
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->muted_ = true;
        this->set_volume_(0);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->muted_ = false;
        this->set_volume_(this->volume);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        float new_volume = this->volume + 0.1f;
        if (new_volume > 1.0f)
          new_volume = 1.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        float new_volume = this->volume - 0.1f;
        if (new_volume < 0.0f)
          new_volume = 0.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
      default:
        break;
    }
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        // if (!this->stream_.isRunning())
        //   if (this->stream_.connecttohost(this->stream_.lastUrl()))
        //     this->state = play_state;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        // if (this->stream_.isRunning()) {
        //   this->stream_.stopSong();
        //   this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        // }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        // this->stream_.stopSong();
        // this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        // if (this->stream_.isRunning()) {
        //   if (this->stream_.connecttohost(this->stream_.lastUrl()))
        //     this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        // } else {
        //   this->stream_.stopSong();
        //   this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        // }
        break;
      default:
        break;
    }
  }
  this->publish_state();
}

void Vs10xxMediaPlayer::set_volume_(float volume, bool publish) {
  // this->stream_.setVolume(remap<uint8_t, float>(volume, 0.0f, 1.0f, 0, 100));
  if (publish)
    this->volume = volume;
}

void Vs10xxMediaPlayer::loop() {
  // if (!ring_buffer_) {
  //   ESP_LOGE(TAG, "Ring buffer is not initialized");
  //   return;
  // }
  size_t total_wrote = 0;
  size_t total_read = 0;

  const auto start_time = micros();

  uint32_t spent_reading = 0, spent_writing = 0;

  while (this->http_container_ && micros() - start_time < 20000) {
    const auto ring_available = std::min<size_t>(512, ring_buffer_->contigious_write());
    if (!ring_available) {
      ESP_LOGVV(TAG, "Buffer is full");
      break;
    }

    ESP_LOGVV(TAG, "Trying to read %d bytes", ring_available);
    const auto start_read = micros();
    const auto read_bytes = this->http_container_->read(ring_buffer_->write, ring_available);
    ring_buffer_->inc_write(read_bytes);
    total_read += read_bytes;
    spent_reading += micros() - start_read;
    ESP_LOGVV(TAG, "Read %d bytes", read_bytes);
    if (read_bytes != ring_available) {
      break;
    }
  }

  Vs10xxAudioComponent::DataWriter writer(*this->parent_);
  while (!ring_buffer_->empty() && micros() - start_time < 20000) {
    const auto ring_available = std::min<size_t>(512, ring_buffer_->contigious_read());
    if (!ring_available) {
      ESP_LOGVV(TAG, "Buffer is empty");
      break;
    }

    ESP_LOGVV(TAG, "Trying to write %d bytes", ring_available);

    const auto start_write = micros();
    const auto wrote_bytes = writer.write(ring_buffer_->read, ring_available);
    ring_buffer_->inc_read(wrote_bytes);
    spent_writing += micros() - start_write;
    total_wrote += wrote_bytes;

    ESP_LOGVV(TAG, "Wrote %d bytes", wrote_bytes);
    if (wrote_bytes != ring_available) {
      break;
    }
  }

  // while (this->http_container_ && parent_->ready_for_data() && micros() - start_time < 20000) {
  //   const auto ring_available = 32;
  //   uint8_t buffer[ring_available];
  //   const auto start_read = micros();
  //   const auto read_bytes = this->http_container_->read(buffer, ring_available);
  //   if (read_bytes == 0)
  //     break;
  //   total_read += read_bytes;
  //   spent_reading += micros() - start_read;

  //   const auto start_write = micros();
  //   const auto wrote_bytes = writer.write(buffer, ring_available);
  //   spent_writing += micros() - start_write;
  //   total_wrote += wrote_bytes;
  // }

  if (total_read || total_wrote) {
    const auto total_spent = micros() - start_time;
    ESP_LOGVV(TAG, "Spent %d writing %d, reading %d bytes (%f Kbps): (reading: %d, writing: %d)", total_spent,
              total_wrote, total_read, float(total_wrote) / float(total_spent) * 1e6f / 1024.0f, spent_reading,
              spent_writing);
  }
}

}  // namespace vs10xx
}  // namespace esphome
