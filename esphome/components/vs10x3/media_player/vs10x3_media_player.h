#pragma once

#include "esphome/components/vs10x3/vs10x3_audio.h"
#include "esphome/components/media_player/media_player.h"

namespace esphome {
namespace vs10x3 {
class Vs10x3MediaPlayer : public Component, public Parented<Vs10x3AudioComponent>, public media_player::MediaPlayer {
 public:
  media_player::MediaPlayerTraits get_traits() override;

 protected:
  void control(const media_player::MediaPlayerCall &call) override;
};

}  // namespace vs10x3
}  // namespace esphome
