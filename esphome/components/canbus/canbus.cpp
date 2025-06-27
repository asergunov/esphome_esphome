#include "canbus.h"

#include <algorithm>
#include "esphome/core/log.h"

namespace esphome {
namespace canbus {

static const char *const TAG = "canbus";

void Canbus::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");
  if (!this->setup_internal()) {
    ESP_LOGE(TAG, "setup error!");
    this->mark_failed();
  }
}

void Canbus::dump_config() {
  if (this->use_extended_id_) {
    ESP_LOGCONFIG(TAG, "config extended id=0x%08" PRIx32, this->can_id_);
  } else {
    ESP_LOGCONFIG(TAG, "config standard id=0x%03" PRIx32, this->can_id_);
  }
}

void Canbus::send_data(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                       const std::vector<uint8_t> &data) {
  struct CanFrame can_message;

  uint8_t size = static_cast<uint8_t>(data.size());
  if (use_extended_id) {
    ESP_LOGD(TAG, "send extended id=0x%08" PRIx32 " rtr=%s size=%d", can_id, TRUEFALSE(remote_transmission_request),
             size);
  } else {
    ESP_LOGD(TAG, "send standard id=0x%03" PRIx32 " rtr=%s size=%d", can_id, TRUEFALSE(remote_transmission_request),
             size);
  }
  size = std::min(size, CAN_MAX_DATA_LENGTH);
  can_message.can_data_length_code = size;
  can_message.can_id = can_id;
  can_message.use_extended_id = use_extended_id;
  can_message.remote_transmission_request = remote_transmission_request;

  for (int i = 0; i < size; i++) {
    can_message.data[i] = data[i];
    ESP_LOGVV(TAG, "  data[%d]=%02x", i, can_message.data[i]);
  }
#ifdef USE_CAN_DEBUGGER
  this->transmit_callback_manager_(can_message.can_id, can_message.use_extended_id,
                                   can_message.remote_transmission_request, data);
#endif
  if (this->send_message(&can_message) != canbus::ERROR_OK) {
    if (use_extended_id) {
      ESP_LOGW(TAG, "send to extended id=0x%08" PRIx32 " failed!", can_id);
    } else {
      ESP_LOGW(TAG, "send to standard id=0x%03" PRIx32 " failed!", can_id);
    }
  }
}

void Canbus::add_trigger(CanbusTrigger *trigger) {
  if (trigger->use_extended_id_) {
    ESP_LOGVV(TAG, "add trigger for extended canid=0x%08" PRIx32, trigger->can_id_);
  } else {
    ESP_LOGVV(TAG, "add trigger for std canid=0x%03" PRIx32, trigger->can_id_);
  }
  this->triggers_.push_back(trigger);
};

void Canbus::loop() {
  struct CanFrame can_message;
  // read all messages until queue is empty
  int message_counter = 0;
  while (this->read_message(&can_message) == canbus::ERROR_OK) {
    message_counter++;
    if (can_message.use_extended_id) {
      ESP_LOGD(TAG, "received can message (#%d) extended can_id=0x%" PRIx32 " size=%d", message_counter,
               can_message.can_id, can_message.can_data_length_code);
    } else {
      ESP_LOGD(TAG, "received can message (#%d) std can_id=0x%" PRIx32 " size=%d", message_counter, can_message.can_id,
               can_message.can_data_length_code);
    }

    std::vector<uint8_t> data;

    // show data received
    for (int i = 0; i < can_message.can_data_length_code; i++) {
      ESP_LOGV(TAG, "  can_message.data[%d]=%02x", i, can_message.data[i]);
      data.push_back(can_message.data[i]);
    }

    this->callback_manager_(can_message.can_id, can_message.use_extended_id, can_message.remote_transmission_request,
                            data);

    // fire all triggers
    for (auto *trigger : this->triggers_) {
      if ((trigger->can_id_ == (can_message.can_id & trigger->can_id_mask_)) &&
          (trigger->use_extended_id_ == can_message.use_extended_id) &&
          (!trigger->remote_transmission_request_.has_value() ||
           trigger->remote_transmission_request_.value() == can_message.remote_transmission_request)) {
        trigger->trigger(data, can_message.can_id, can_message.remote_transmission_request);
      }
    }
  }
}
constexpr uint32_t BITS_IN_KBIT = 1000;
static constexpr std::array<uint32_t, 21> BITS_PER_SECOND{
    BITS_IN_KBIT * 1,    BITS_IN_KBIT * 5,   BITS_IN_KBIT * 10,  BITS_IN_KBIT * 12,
    BITS_IN_KBIT * 16,   BITS_IN_KBIT * 20,  BITS_IN_KBIT * 25,  (BITS_IN_KBIT * 31) + (BITS_IN_KBIT / 4),
    BITS_IN_KBIT * 33,   BITS_IN_KBIT * 40,  BITS_IN_KBIT * 50,  BITS_IN_KBIT * 80,
    BITS_IN_KBIT * 83,   BITS_IN_KBIT * 95,  BITS_IN_KBIT * 100, BITS_IN_KBIT * 125,
    BITS_IN_KBIT * 200,  BITS_IN_KBIT * 250, BITS_IN_KBIT * 500, BITS_IN_KBIT * 800,
    BITS_IN_KBIT * 1000,
};

uint32_t Canbus::set_bits_per_second(uint32_t arg) {
  const auto *i = std::ranges::lower_bound(BITS_PER_SECOND, arg);
  if (i == BITS_PER_SECOND.end()) {
    --i;
  }
  const auto fixed_bitrate = static_cast<CanSpeed>(std::distance(BITS_PER_SECOND.begin(), i));
  set_bitrate(fixed_bitrate);
  return *i;
}

uint32_t Canbus::get_bits_per_second() const { return BITS_PER_SECOND[this->get_bitrate()]; }

}  // namespace canbus
}  // namespace esphome
