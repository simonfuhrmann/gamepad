#include "gamepad.h"

#include <iostream>
#include <map>

#include "gamepad_linux.h"
#include "gamepad_libstem.h"

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

}  // namespace gamepad
