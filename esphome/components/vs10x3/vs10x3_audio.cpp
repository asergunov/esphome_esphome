#include "vs10x3_audio.h"

namespace esphome {
namespace vs10x3 {

static const char *const TAG = "vs10x3";

void IRAM_ATTR HOT s_gpio_intr_rise(Vs10x3AudioComponent::InterruptData *self) {
  self->dreq_was_active_ = true;
  self->dreq_is_active_ = true;
}
void IRAM_ATTR HOT s_gpio_intr_fall(Vs10x3AudioComponent::InterruptData *self) { self->dreq_is_active_ = false; }

void Vs10x3AudioComponent::setup() {
  ESP_LOGV(TAG, "Setting up vs10x3");

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

void Vs10x3AudioComponent::DataDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "   data_device:");
  LOG_PIN("     cs_pin: ", this->cs_);
}

void Vs10x3AudioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "vs10x3:");
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

void Vs10x3AudioComponent::loop() {
  if (this->in_hw_reset_)
    if (!handle_reset_())
      return;

  // const auto new_mode = this->read_register_(SCI_MODE);
  // if (new_mode != mode_) {
  //   ESP_LOGV(TAG, "Chip mode has changed from %04X to %04X", mode_, new_mode);
  //   mode_ = new_mode;
  // }
  // const auto new_status = this->read_register_(SCI_STATUS);
  // if (new_status != status_) {
  //   ESP_LOGV(TAG, "Chip status has changed from %04X to %04X", status_, new_status);
  //   status_ = new_status;
  //   version_ = (status_ >> 4) & 0b111;
  //   ESP_LOGV(TAG, "Chip version is %d", version_);
  // }
}

bool Vs10x3AudioComponent::handle_reset_() {
  if (!in_hw_reset_)
    return true;

  const auto worst_boot_time = this->xtali_to_micros_(50000);
  if (this->dreq_pin_->digital_read()) {
    this->in_hw_reset_ = false;
    ESP_LOGI(TAG, "Reset completed in %luus of %luus", micros() - hw_reset_start_micros_, worst_boot_time);
    ESP_LOGV(TAG, "Reding mode after hw reset");

    mode_ = this->read_register_(SCI_MODE);
    ESP_LOGV(TAG, "Chip mode is %04X", mode_);

    for (status_ = this->read_register_(SCI_STATUS); (status_ & 0b1111) == 0xC;
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
    ESP_LOGV(TAG, "Writing clockf %04x", clockf_);
    this->write_register_(SCI_CLOCKF, clockf_);
    vs1xxx_registers::SC_MULT mult;
    clockf_ >> mult;
    this->clki_ = mult.clki_for_xtali(this->xtali_);

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

void Vs10x3AudioComponent::wait_sci_ready_() {
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

void Vs10x3AudioComponent::write_register_(Register r, uint16_t value) {
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

uint16_t Vs10x3AudioComponent::read_register_(Register r) {
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

}  // namespace vs10x3
}  // namespace esphome
