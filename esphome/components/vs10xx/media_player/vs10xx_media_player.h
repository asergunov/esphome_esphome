#pragma once

#include "esphome/components/vs10xx/vs10xx_audio.h"
#include "esphome/components/media_player/media_player.h"
#include "esphome/components/http_request/http_request.h"

namespace esphome {
namespace vs10xx {
class Vs10xxMediaPlayer : public Component, public Parented<Vs10xxAudioComponent>, public media_player::MediaPlayer {
 public:
  void setup() override;
  void loop() override;
  media_player::MediaPlayerTraits get_traits() override;
  void set_http_request(http_request::HttpRequestComponent *http_request) { http_request_ = http_request; }

 protected:
  void control(const media_player::MediaPlayerCall &call) override;

  void set_volume_(float volume, bool publish = true);
  void mute_() { this->muted_ = true; }
  void unmute_() { this->muted_ = false; }

 protected:
  bool is_announcement_ = false;
  bool muted_ = false;
  http_request::HttpRequestComponent *http_request_;
  std::shared_ptr<http_request::HttpContainer> http_container_;
  HighFrequencyLoopRequester high_freq_;

  template<size_t SIZE> struct RingBufferT {
    uint8_t data[SIZE];
    uint8_t *read;
    uint8_t *write;

    RingBufferT() { reset(); }

    void reset() {
      read = data;
      write = data;
    }

    void inc_read(size_t count) { read = data + (read - data + count) % SIZE; }
    void inc_write(size_t count) { write = data + (write - data + count) % SIZE; }
    size_t stored() { return read <= write ? write - read : write + SIZE - read; }
    size_t available() { return SIZE - stored() - 1; }
    bool empty() { return read == write; }
    bool full() { return (read - write) % SIZE == 1; }
    size_t contigious_read() { return empty() ? 0 : read < write ? write - read : data + SIZE - read; }
    size_t contigious_write() {
      return full()         ? 0
             : read > write ? read - write - 1
             : read != data ? data + SIZE - write
                            : data + SIZE - write - 1;
    }
  };

  using RingBuffer = RingBufferT<64 * 1024>;
  std::unique_ptr<RingBuffer> ring_buffer_;
};

}  // namespace vs10xx
}  // namespace esphome
