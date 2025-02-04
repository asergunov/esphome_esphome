#include "vs10x3_media_player.h"

namespace esphome {
namespace vs10x3 {

void Vs10x3MediaPlayer::setup() { this->stream_.attachDecoder(this->parent_->get_decoder()); }

void Vs10x3MediaPlayer::loop() { this->stream_.loop(); }

media_player::MediaPlayerTraits Vs10x3MediaPlayer::get_traits() {
  media_player::MediaPlayerTraits traits;
  traits.set_supports_pause(false);
  return traits;
}

void Vs10x3MediaPlayer::control(const media_player::MediaPlayerCall &call) {
  media_player::MediaPlayerState play_state = media_player::MEDIA_PLAYER_STATE_PLAYING;
  if (call.get_announcement().has_value()) {
    play_state = call.get_announcement().value() ? media_player::MEDIA_PLAYER_STATE_ANNOUNCING
                                                 : media_player::MEDIA_PLAYER_STATE_PLAYING;
  }

  const auto &media_url = call.get_media_url();
  if (media_url.has_value()) {
    if (this->stream_.connecttohost(media_url->c_str())) {
      this->state = play_state;
    } else {
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
    }
  }

  if (play_state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING) {
    this->is_announcement_ = true;
  }

  if (call.get_volume().has_value()) {
    this->set_volume_(call.get_volume().value());
    this->unmute_();
  }
  if (call.get_command().has_value()) {
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
        if (!this->stream_.isRunning())
          if (this->stream_.connecttohost(this->stream_.lastUrl()))
            this->state = play_state;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        if (this->stream_.isRunning()) {
          this->stream_.stopSong();
          this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        this->stream_.stopSong();
        this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        if (this->stream_.isRunning()) {
          if (this->stream_.connecttohost(this->stream_.lastUrl()))
            this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        } else {
          this->stream_.stopSong();
          this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        }
        break;
      default:
        break;
    }
  }
  this->publish_state();
}

void Vs10x3MediaPlayer::set_volume_(float volume, bool publish) {
  this->stream_.setVolume(remap<uint8_t, float>(volume, 0.0f, 1.0f, 0, 100));
  if (publish)
    this->volume = volume;
}

}  // namespace vs10x3
}  // namespace esphome
