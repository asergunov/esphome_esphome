#include "vs10x3_audio.h"

namespace esphome {
namespace vs10x3 {

static const char *const TAG = "vs10x3";

void Vs10x3AudioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up vs10x3");
  // this->spi_setup(); // creashes

  this->vs1053_ = make_unique<VS1053>(this->cs_pin_, this->dcs_pin_, this->dreq_pin_);
  this->vs1053_->begin();
}

void Vs10x3AudioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "vs10x3:");
  ESP_LOGCONFIG(TAG, "   cs_pin: %d", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "   dcs_pin: %d", this->dcs_pin_);
  ESP_LOGCONFIG(TAG, "   dreq_pin: %d", this->dreq_pin_);
}

}  // namespace vs10x3
}  // namespace esphome
