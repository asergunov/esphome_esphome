#pragma once

#include "esphome/components/display_7segment_base/display.h"
#include "esphome/components/display_menu_base/display_menu_base.h"

#include <forward_list>
#include <vector>

namespace esphome {
namespace lcd_menu_7segment {

/** Class to display a hierarchical menu.
 *
 */
class LCD7SegmentMenuComponent : public display_menu_base::DisplayMenuComponent {
 public:
  void set_display(display_7segment_base::Display *display) { this->display_ = display; }
  void set_length(uint8_t columns) {
    this->columns_ = columns;
    set_rows(1);
  }

  void setup() override;
  float get_setup_priority() const override;

  void dump_config() override;

 protected:
  void draw_item(const display_menu_base::MenuItem *item, uint8_t row, bool selected) override;
  void update() override { this->display_->update(); }

  display_7segment_base::Display *display_;
  uint8_t columns_;
};

}  // namespace lcd_menu_7segment
}  // namespace esphome
