#include "lcd_menu.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace lcd_menu {

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
  ESP_LOGCONFIG(TAG, "  Mark characters: %02x, %02x, %02x", this->mark_editing_, this->mark_submenu_, this->mark_back_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "The connected display failed, the menu is disabled!");
  }
}

void LCD7SegmentMenuComponent::draw_item(const display_menu_base::MenuItem *item, uint8_t row, bool selected) {
  char data[this->columns_ + 1];  // Bounded to 65 through the config

  memset(data, ' ', this->columns_);

  if (!selected) {
    return;
  }

  switch (item->get_type()) {
    case display_menu_base::MENU_ITEM_MENU:
      data[this->columns_ - 1] = this->mark_submenu_;
      break;
    case display_menu_base::MENU_ITEM_BACK:
      data[this->columns_ - 1] = this->mark_back_;
      break;
    default:
      break;
  }

  const auto &text = item->get_text();
  size_t n = std::min(text.size(), (size_t) this->columns_ - 2);
  memcpy(data + 1, text.c_str(), n);

  // if (item->has_value()) {
  //   std::string value = item->get_value_text();

  //   // Maximum: start mark, at least two chars of label, space, '[', value, ']',
  //   // end mark. Config guarantees columns >= 12
  //   size_t val_width = std::min((size_t) this->columns_ - 7, value.length());
  //   memcpy(data + this->columns_ - val_width - 4, " [", 2);
  //   memcpy(data + this->columns_ - val_width - 2, value.c_str(), val_width);
  //   data[this->columns_ - 2] = ']';
  // }

  data[this->columns_] = '\0';

  this->display_->print(0, data);
}

}  // namespace lcd_menu
}  // namespace esphome
