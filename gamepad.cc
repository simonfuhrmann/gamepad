/*
 * Written by Simon Fuhrmann.
 * See LICENSE file for details.
 */
#include "gamepad.h"

#include <iostream>
#include <map>

#include "gamepad_libstem.h"
#include "gamepad_linux.h"
#include "gamepad_osx.h"

namespace gamepad {

std::unique_ptr<System>
System::Create() {
  return std::unique_ptr<System>(new SystemImpl());
}

void
System::RegisterAttachHandler(AttachedHandler handler) {
  attached_handler_ = handler;
}

void
System::RegisterDetachHandler(DetachedHandler handler) {
  detached_handler_ = handler;
}

void
System::RegisterButtonDownHandler(ButtonHandler handler) {
  button_down_handler_ = handler;
}

void
System::RegisterButtonUpHandler(ButtonHandler handler) {
  button_up_handler_ = handler;
}

void
System::RegisterAxisMoveHandler(AxisHandler handler) {
  axis_move_handler_ = handler;
}

void
System::HandleButtonEvent(Device* device, int button_id, int value) {
  const bool is_down = value > 0;
  device->buttons[button_id] = is_down;
  if (is_down && button_down_handler_) {
    button_down_handler_(device, button_id, 0.0);
  } else if (!is_down && button_up_handler_) {
    button_up_handler_(device, button_id, 0.0);
  }
}

void
System::HandleAxisEvent(Device* device, int axis_id, int value,
    int min, int max, int fuzz, int flat) {
  // Flatten value. Values within flat-range will be reported as zero.
  value = value > -flat && value < flat ? 0 : value;
  // Normalize value and camp value to [-1, 1].
  const float minimum = static_cast<float>(min);
  const float maximum = static_cast<float>(max);
  const float range = maximum - minimum;
  const float norm = (static_cast<float>(value) - minimum) / range;
  const float clamped = std::max(-1.0f, std::min(1.0f, 2.0f * norm - 1.0f));
  // Send an update if the new value is different from the last value. Use an
  // epsilon comparison to the last value based on the fuzz value.
  const float last = device->axes[axis_id];
  const float eps = static_cast<float>(2 * fuzz) / range;
  if (clamped > last + eps || clamped < last - eps) {
    device->axes[axis_id] = clamped;
    if (axis_move_handler_) {
      axis_move_handler_(device, axis_id, clamped, last, 0.0);
    }
  }
}

}  // namespace gamepad
