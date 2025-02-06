#include "vs10x3_audio.h"

namespace esphome {
namespace vs10x3 {

static const char *const TAG = "vs10x3";

void Vs10x3AudioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up vs10x3");
  this->spi_setup();  // creashes

  ESP_LOGV(TAG, "Creating driver");
  this->vs1053_ = make_unique<VS1053>(this->cs_pin_, this->dcs_pin_, this->dreq_pin_);
  ESP_LOGV(TAG, "Initializing");
  this->vs1053_->begin();

  if (this->vs1053_->isChipConnected()) {
    ESP_LOGI(TAG, "Device connected. Version is %d", this->vs1053_->getChipVersion());
  } else {
    ESP_LOGE(TAG, "Device is not connected");
  }
}

void Vs10x3AudioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "vs10x3:");
  ESP_LOGCONFIG(TAG, "   cs_pin: %d", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "   dcs_pin: %d", this->dcs_pin_);
  ESP_LOGCONFIG(TAG, "   dreq_pin: %d", this->dreq_pin_);

  if (this->vs1053_) {
    ESP_LOGCONFIG(TAG, "   chip conected: %d", this->vs1053_->isChipConnected());
    ESP_LOGCONFIG(TAG, "   chip version: %d", this->vs1053_->getChipVersion());
  }
}

}  // namespace vs10x3
}  // namespace esphome
