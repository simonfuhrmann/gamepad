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

}  // namespace gamepad
