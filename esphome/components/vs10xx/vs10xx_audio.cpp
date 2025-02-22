#include "vs10xx_audio.h"

namespace esphome {
namespace vs10xx {

static const char *const TAG = "vs10xx";

void IRAM_ATTR HOT s_gpio_intr_rise(Vs10xxAudioComponent::InterruptData *self) {
  self->dreq_was_active_ = true;
  self->dreq_is_active_ = true;
}
void IRAM_ATTR HOT s_gpio_intr_fall(Vs10xxAudioComponent::InterruptData *self) { self->dreq_is_active_ = false; }

void Vs10xxAudioComponent::setup() {
  ESP_LOGV(TAG, "Setting up %s", TAG);

  if (this->reset_pin_)
    this->reset_pin_->setup();
  this->dreq_pin_->setup();

  this->spi_setup();
  this->data_device_.spi_setup();

  this->interrupt_data_.dreq_is_active_ = this->dreq_pin_->digital_read();
  this->dreq_pin_->attach_interrupt(&s_gpio_intr_rise, &this->interrupt_data_, gpio::INTERRUPT_RISING_EDGE);
  this->dreq_pin_->attach_interrupt(&s_gpio_intr_fall, &this->interrupt_data_, gpio::INTERRUPT_FALLING_EDGE);

  if (this->reset_pin_) {
    this->reset_pin_->digital_write(true);
  }
  this->hw_reset_start_micros_ = micros();
  this->in_hw_reset_ = true;
  this->handle_reset_();
}

void Vs10xxAudioComponent::DataDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "   data_device:");
  LOG_PIN("     cs_pin: ", this->cs_);
}

void Vs10xxAudioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "%s:", TAG);
  LOG_PIN("   cs_pin: ", this->cs_);
  LOG_PIN("   reset_pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "   version: %d", this->version_);
  ESP_LOGCONFIG(TAG, "   mode: %04X", this->mode_);
  ESP_LOGCONFIG(TAG, "   status: %04X", this->status_);
  ESP_LOGCONFIG(TAG, "   clockf: %04X", this->clockf_);
  ESP_LOGCONFIG(TAG, "   xtali: %d", this->xtali_);
  ESP_LOGCONFIG(TAG, "   clki: %d", this->clki_);
  this->data_device_.dump_config();
}

void Vs10xxAudioComponent::loop() {
  if (this->in_hw_reset_)
    if (!handle_reset_())
      return;

  refresh_regiter_(mode_);
  refresh_regiter_(status_);
  refresh_regiter_(clockf_);
  if (refresh_regiter_(audata_)) {
    decltype(audata_)::SC_HALF_SAMPLERATE sr;
    audata_ >> sr;
    ESP_LOGV(TAG,
             "Audio data: sample rate: %d (mask: %d, begin_bit: %d, end_bit: %d), stereo: %d (mask: %d, begin_bit: %d, "
             "end_bit: %d), sr: %d",
             int(audata_.sample_rate()), int(decltype(audata_)::SC_HALF_SAMPLERATE::MASK),
             int(decltype(audata_)::SC_HALF_SAMPLERATE::BEGIN_BIT), int(decltype(audata_)::SC_HALF_SAMPLERATE::END_BIT),
             int(audata_.is_stereo()), int(decltype(audata_)::SC_CHANNELS::MASK),
             int(decltype(audata_)::SC_CHANNELS::BEGIN_BIT), int(decltype(audata_)::SC_CHANNELS::END_BIT), int(sr));
  }
  refresh_regiter_(decode_time_);
  const auto hdat1_changed = refresh_regiter_(hdat1_);
  const auto hdat0_changed = refresh_regiter_(hdat0_);
  if (hdat1_changed || hdat0_changed) {
    ESP_LOGV(TAG, "protected: %d (mask: %d, begin bit: %d, end_bit: %d),",
             int(hdat1_.read_field<decltype(hdat1_)::MP3_PROTECT_BIT>()), int(decltype(hdat1_)::MP3_PROTECT_BIT::MASK),
             int(decltype(hdat1_)::MP3_PROTECT_BIT::BEGIN_BIT), int(decltype(hdat1_)::MP3_PROTECT_BIT::END_BIT));
    switch (hdat1_.stream_type()) {
      case vs1xxx_registers::StreamType::None:
        ESP_LOGV(TAG, "Stream type: None");
        break;
      case vs1xxx_registers::StreamType::Wav:
        ESP_LOGV(TAG, "Stream type: Wav");
        break;
      case vs1xxx_registers::StreamType::Wma:
        ESP_LOGV(TAG, "Stream type: Wma, bitrate: %d", hdat0_.wma_bitrate());
        break;
      case vs1xxx_registers::StreamType::Mp3: {
        const auto id = hdat1_.read_field<decltype(hdat1_)::MP3_ID>();
        const auto layer = hdat1_.read_field<decltype(hdat1_)::MP3_LAYER>();
        ESP_LOGV(TAG,
                 "Stream type: Mp3, id: %s, layer %s, bitrate: %d, samplerate: %d, protected: %d, pad: %d, mode: %s, "
                 "extension: %d, copyright: %d, original: %d, emphasis: %s",
                 id.name(), layer.name(), int(hdat0_.mp3_bitrate(id)), int(hdat0_.mp3_sample_rate(id)),
                 int(hdat1_.read_field<decltype(hdat1_)::MP3_PROTECT_BIT>()),
                 int(hdat0_.read_field<decltype(hdat0_)::MP3_PAD_BIT>()),
                 hdat0_.read_field<decltype(hdat0_)::MP3_MODE>().name(),
                 int(hdat0_.read_field<decltype(hdat0_)::MP3_EXTENSION>()),
                 int(hdat0_.read_field<decltype(hdat0_)::MP3_COPYRIGHT>()),
                 int(hdat0_.read_field<decltype(hdat0_)::MP3_ORIGINAL>()),
                 hdat0_.read_field<decltype(hdat0_)::MP3_EMPHASIS>().name());
      } break;
      case vs1xxx_registers::StreamType::Midi: {
        ESP_LOGV(TAG, "Stream type: Midi");
      }
      default: {
        ESP_LOGW(TAG, "Stream type: Unknown");
      }
    }
  }
}

bool Vs10xxAudioComponent::handle_reset_() {
  if (!in_hw_reset_)
    return true;

  const auto worst_boot_time = this->xtali_to_micros_(50000);
  if (this->dreq_pin_->digital_read()) {
    this->in_hw_reset_ = false;
    ESP_LOGI(TAG, "Reset completed in %luus of %luus", micros() - hw_reset_start_micros_, worst_boot_time);
    ESP_LOGV(TAG, "Reding mode after hw reset");

    mode_ = this->read_register_(SCI_MODE);
    ESP_LOGV(TAG, "Chip mode is %04X", mode_);
    // mode_ << vs1xxx_registers::SM_STREAM(1);
    write_register_(mode_);

    for (status_ = this->read_register_(SCI_STATUS); (status_.value() & 0b1111) == 0xC;
         status_ = this->read_register_(SCI_STATUS)) {
      ESP_LOGV(TAG,
               "Chip status is %04X. Reading status again. VS1003 was seen returning 0x000C in short time on startup.",
               status_);
    }
    ESP_LOGV(TAG, "Chip status is %04X", status_);
    status_ >> version_;
    ESP_LOGV(TAG, "Chip is %s (version: %d)", version_.chip_name(), version_);

    if (version_ != 3) {
      ESP_LOGW(TAG, "%s (version: %d) is not supported", version_.chip_name(), version_);
      this->status_set_error(str_sprintf("Unsupported chip 0x%02X", this->version_).c_str());
    }

    clockf_ << vs1xxx_registers::SC_FREQ::from_hz(this->xtali_);
    ESP_LOGV(TAG, "Writing clockf %04x", clockf_.value());
    this->write_register_(SCI_CLOCKF, clockf_.value());
    update_clki_();

    return true;
  }

  if (micros() - hw_reset_start_micros_ < worst_boot_time) {
    ESP_LOGVV(TAG, "Waiting chip to boot %luus of %luus", micros() - hw_reset_start_micros_, worst_boot_time);
    return false;
  }

  if (!reset_pin_) {
    this->status_set_error(
        "Chip is not responding on time after hw boot. Check dreq_pin or specify reset_pin to retry.");
    return false;
  }

  ESP_LOGW(TAG, "Chip is not responting for %luus of %luus. Performing reset again. Check dreq_pin.",
           micros() - hw_reset_start_micros_, worst_boot_time);
  reset_pin_->digital_write(false);
  delayMicroseconds(this->xtali_to_micros_(2) + 1);
  reset_pin_->digital_write(true);
  hw_reset_start_micros_ = micros();
  return false;
}

void Vs10xxAudioComponent::wait_sci_ready_() {
  if (last_command_worst_duration_ == 0) {
    ESP_LOGVV(TAG, "Comand was not sent");
    // Don't have anything executing
    return;
  }
  const auto time_since_last_command = micros() - last_command_sent_micros_;
  ESP_LOGVV(TAG, "Last command was sent %dus ago. Worst duration %dus", time_since_last_command,
            last_command_worst_duration_);
  if (time_since_last_command <= 1) {
    ESP_LOGVV(TAG, "Wait at least 2 clki of inactive cs to send the next command");
    delayMicroseconds(1);
  }

  const auto time_to_send_command = 1000000 * 32 / this->data_rate_;  // 32 bits to send over SCI
  if (last_command_worst_duration_ < time_to_send_command) {
    ESP_LOGVV(TAG, "Current command will be done in %dus before the end of transmission %dus",
              last_command_worst_duration_, time_to_send_command);
    return;
  }

  const auto micros_to_wait = last_command_worst_duration_ - time_to_send_command;
  ESP_LOGVV(TAG, "Waiting %dus or dreq active.", micros_to_wait);
  while (!this->dreq_is_active_() && micros() - last_command_sent_micros_ < micros_to_wait) {
    delayMicroseconds(1);
  }
}

void Vs10xxAudioComponent::write_register_(Register r, uint16_t value) {
  wait_sci_ready_();
  this->enable();
  const auto &reg = static_cast<uint8_t>(r);
  const uint8_t data[4] = {0b10, reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
  this->write_array(data, sizeof(data));
  this->disable();

  this->last_command_sent_micros_ = micros();
  const auto &delay = registerWriteWorstDelay[reg];
  this->last_command_worst_duration_ = 1 + this->delay_to_micros_(delay);
}

uint16_t Vs10xxAudioComponent::read_register_(Register r) {
  wait_sci_ready_();
  this->enable();
  const uint8_t data[2] = {0b11, static_cast<uint8_t>(r)};
  this->write_array(data, sizeof(data));

  uint8_t result[2];
  this->read_array(result, sizeof(result));
  this->disable();

  this->last_command_sent_micros_ = micros();
  this->last_command_worst_duration_ = 1;

  return static_cast<uint16_t>(result[0]) << 8 | result[1];
}

void Vs10xxAudioComponent::update_clki_() {
  vs1xxx_registers::SC_MULT mult;
  clockf_ >> mult;
  this->clki_ = mult.clki_for_xtali(this->xtali_);

  ESP_LOGV(TAG, "CLKI is %d", this->clki_);

  this->spi_teardown();
  this->data_device_.spi_teardown();

  this->set_data_rate(this->clki_ / 7);
  this->data_device_.set_data_rate(this->clki_ / 4);

  ESP_LOGV(TAG, "Command interface speed %d, Data interface speed is %d", this->data_rate_,
           this->data_device_.data_rate_);

  this->spi_setup();
  this->data_device_.spi_setup();
}

}  // namespace vs10xx
}  // namespace esphome
