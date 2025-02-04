#pragma once

#include "esphome/components/vs10x3/vs10x3_audio.h"
#include "esphome/components/media_player/media_player.h"

#include <ESP32_VS1053_Stream.h>

namespace esphome {
namespace vs10x3 {
class Vs10x3MediaPlayer : public Component, public Parented<Vs10x3AudioComponent>, public media_player::MediaPlayer {
 public:
  void setup() override;
  void loop() override;
  media_player::MediaPlayerTraits get_traits() override;

 protected:
  void control(const media_player::MediaPlayerCall &call) override;

  void set_volume_(float volume, bool publish = true);
  void mute_() { this->muted_ = true; }
  void unmute_() { this->muted_ = false; }

 protected:
  ESP32_VS1053_Stream stream_;
  bool is_announcement_ = false;
  bool muted_ = false;
};

}  // namespace vs10x3
}  // namespace esphome
