#include "vs10x3_media_player.h"

namespace esphome {
namespace vs10x3 {

media_player::MediaPlayerTraits Vs10x3MediaPlayer::get_traits() {
  media_player::MediaPlayerTraits ret;
  return ret;
}

void Vs10x3MediaPlayer::control(const media_player::MediaPlayerCall &call) {}

}  // namespace vs10x3
}  // namespace esphome
