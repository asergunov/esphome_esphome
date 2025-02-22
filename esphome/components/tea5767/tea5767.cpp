#include "tea5767.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tea5767 {
static const char *const TAG = "tea5767";

void Tea5767Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up tea5767...");

  write_();
  read_();
}

void Tea5767Component::dump_config() {
  ESP_LOGCONFIG(TAG, "TEA5767:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(
      TAG,
      "  config: mute: %d, search: %d, pll: %d, search_up: %d, hilo: %d, mono: %d, mute_left: %d, "
      "mute_right: %d, swp1: %d, swp2: %d, stby: %d, band_jp: %d, clock_freq: %s, external pll ref: %d",
      config_.get<Config::Mute>(), config_.get<Config::SearchMode>(), config_.get<PLL<Config>>(),
      config_.get<Config::SearchUp>(), config_.get<Config::HightSideInjectionLO>(), config_.get<Config::ForceMono>(),
      config_.get<Config::MuteLeft>(), config_.get<Config::MuteRight>(), config_.get<Config::SWP1>(),
      config_.get<Config::SWP2>(), config_.get<Config::Standby>(), config_.get<Config::BandLimits>(),
      [&] {
        switch (config_.get<Config::XTAL>()) {
          case CF_13MHz:
            return "13MHz";
          case CF_32768Hz:
            return "32.768kHz";
          default:
            return "not allowed";
        }
      }(),
      int(config_.get<Config::PLLREF>()));
  ESP_LOGCONFIG(TAG, "    soft_mute: %d", config_.get<Config::SoftMute>());
  ESP_LOGCONFIG(TAG, "    hight_cut: %d", config_.get<Config::HighCutControl>());
  ESP_LOGCONFIG(TAG, "    snc: %d", config_.get<Config::StereoNoiseCancelation>());
  ESP_LOGCONFIG(TAG, "    search_indicator: %d", config_.get<Config::SearchIndicator>());
  ESP_LOGCONFIG(TAG, "    dts: %s", config_.get<Config::DeEmphasisTimeConstant>() == DTC::DTC_50us ? "50us" : "75us");

  ESP_LOGCONFIG(TAG, "  status:");
  ESP_LOGCONFIG(TAG, "    search_done: %d", status_.get<Status::ReadyFlag>());
  ESP_LOGCONFIG(TAG, "    search_band_limit: %d", status_.get<Status::BandLimitFlag>());
  ESP_LOGCONFIG(TAG, "    pll: %d", status_.get<PLL<Status>>());
  ESP_LOGCONFIG(TAG, "    stereo: %d", status_.get<Status::Stereo>());
  ESP_LOGCONFIG(TAG, "    if: %d", status_.get<Status::IF>());
  ESP_LOGCONFIG(TAG, "    lev: %d", status_.get<Status::LEV>());
  ESP_LOGCONFIG(TAG, "    ci: %d", status_.get<Status::CI>());
}

void Tea5767Component::loop() {
  read_();
  config_.set<PLL<Config>>(status_.get<PLL<Status>>(), false);
  if (config_.dirty_)
    write_();
}

void Tea5767Component::write_() {
  ESP_LOGV(TAG, "Writing %01X %01X %01X %01X %01X", config_[0], config_[1], config_[2], config_[3], config_[4]);
  const auto errorCode = I2CDevice::write(config_.data(), config_.size());
  if (errorCode != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "i2c write error: %d", int(errorCode));
    return;
  }
  config_.dirty_ = false;
}
void Tea5767Component::read_() {
  const auto old = status_;
  const auto errorCode = I2CDevice::read(status_.data(), status_.size());
  if (errorCode != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "i2c read error: %d", int(errorCode));
    return;
  }
  ESP_LOGVV(TAG, "Read %01X %01X %01X %01X %01X", status_[0], status_[1], status_[2], status_[3], status_[4]);
  if (status_ != old) {
    ESP_LOGV(TAG, "Changed %01X %01X %01X %01X %01X -> %01X %01X %01X %01X %01X", old[0], old[1], old[2], old[3],
             old[4], status_[0], status_[1], status_[2], status_[3], status_[4]);
    ESP_LOGV(TAG, "  search_done: %d", status_.get<Status::ReadyFlag>());
    ESP_LOGV(TAG, "  search_band_limit: %d", status_.get<Status::BandLimitFlag>());
    ESP_LOGV(TAG, "  pll: %d", status_.get<PLL<Status>>());
    ESP_LOGV(TAG, "  stereo: %d", status_.get<Status::Stereo>());
    ESP_LOGV(TAG, "  if: %d", status_.get<Status::IF>());
    ESP_LOGV(TAG, "  lev: %d", status_.get<Status::LEV>());
    ESP_LOGV(TAG, "  ci: %d", status_.get<Status::CI>());
    ESP_LOGV(TAG, "  freq: %f", config_.pll_to_freq(status_.get<PLL<Status>>()));
  }
}

void Tea5767Component::seek_up() {
  ESP_LOGV(TAG, "Seeking up");
  config_.set<Config::SearchMode>(true);
  config_.set<Config::SearchUp>(true);
}
void Tea5767Component::seek_down() {
  ESP_LOGV(TAG, "Seeking down");
  config_.set<Config::SearchMode>(true);
  config_.set<Config::SearchUp>(false);
}

}  // namespace tea5767
}  // namespace esphome
