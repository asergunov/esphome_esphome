#include "lcd_menu_7segment.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace lcd_menu_7segment {

static const char *const TAG = "7segment_menu";

void LCD7SegmentMenuComponent::setup() {
  if (this->display_->is_failed()) {
    this->mark_failed();
    return;
  }

  display_menu_base::DisplayMenuComponent::setup();
}

float LCD7SegmentMenuComponent::get_setup_priority() const { return setup_priority::PROCESSOR - 1.0f; }

void LCD7SegmentMenuComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "7segment Menu");
  ESP_LOGCONFIG(TAG, "  Columns: %u, Rows: %u", this->columns_, this->rows_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "The connected display failed, the menu is disabled!");
  }
}

void LCD7SegmentMenuComponent::draw_item(const display_menu_base::MenuItem *item, uint8_t row, bool selected) {
  const size_t data_size = this->columns_ * 4;
  char data[data_size + 1];  // Bounded to 65 through the config

  memset(data, ' ', data_size);

  if (!selected) {
    return;
  }

  const auto &text = this->editing_ ? item->get_value_text() : item->get_text();
  size_t n = std::min(text.size(), data_size);
  memcpy(data, text.c_str(), n);
  // if (item->has_value()) {
  //   std::string value = item->get_value_text();

  //   // Maximum: start mark, at least two chars of label, space, '[', value, ']',
  //   // end mark. Config guarantees columns >= 12
  //   size_t val_width = std::min((size_t) this->columns_ - 7, value.length());
  //   memcpy(data + this->columns_ - val_width - 4, " [", 2);
  //   memcpy(data + this->columns_ - val_width - 2, value.c_str(), val_width);
  //   data[this->columns_ - 2] = ']';
  // }

  data[n] = '\0';

  this->display_->print(0, data);
}

}  // namespace lcd_menu_7segment
}  // namespace esphome
