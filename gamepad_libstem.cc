#if defined(_WIN32) || defined(__APPLE__)

#include "gamepad_libstem.h"

namespace gamepad {

void
SystemImpl::ProcessEvents() {
  if (!initialized_) {
    Gamepad_deviceAttachFunc(SystemImpl::OnAttached, this);
    Gamepad_deviceRemoveFunc(SystemImpl::OnDetached, this);
    Gamepad_buttonDownFunc(SystemImpl::OnButtonDown, this);
    Gamepad_buttonUpFunc(SystemImpl::OnButtonUp, this);
    Gamepad_axisMoveFunc(SystemImpl::OnAxisMove, this);
    Gamepad_init();
    initialized_ = true;
  }
  Gamepad_detectDevices();
  Gamepad_processEvents();
}

void
SystemImpl::CreateDevice(Gamepad_device* device) {
  Device dev;
  dev.device_id = device->deviceID;
  dev.description = device->description;
  dev.vendor_id = device->vendorID;
  dev.product_id = device->productID;
  dev.axes.resize(device->numAxes, 0.0f);
  dev.buttons.resize(device->numButtons, false);
  devices_[device] = dev;
}

void
SystemImpl::OnAttached(Gamepad_device* device, void* context) {
  SystemImpl* obj = static_cast<SystemImpl*>(context);
  obj->CreateDevice(device);
  if (obj->attached_handler_) {
    obj->attached_handler_(&obj->devices_[device]);
  }
}

void
SystemImpl::OnDetached(Gamepad_device* device, void* context) {
  SystemImpl* obj = static_cast<SystemImpl*>(context);
  if (obj->detached_handler_) {
    obj->detached_handler_(&obj->devices_[device]);
  }
}

void
SystemImpl::OnButtonDown(Gamepad_device* device, unsigned int buttonId, double timestamp, void* context) {
  SystemImpl* obj = static_cast<SystemImpl*>(context);
  Device* dev = &obj->devices_[device];
  dev->buttons[buttonId] = true;
  if (obj->button_down_handler_) {
    obj->button_down_handler_(dev, buttonId, timestamp);
  }
}

void
SystemImpl::OnButtonUp(Gamepad_device* device, unsigned int buttonId, double timestamp, void* context) {
  SystemImpl* obj = static_cast<SystemImpl*>(context);
  Device* dev = &obj->devices_[device];
  dev->buttons[buttonId] = false;
  if (obj->button_up_handler_) {
    obj->button_up_handler_(dev, buttonId, timestamp);
  }
}

void
SystemImpl::OnAxisMove(Gamepad_device* device, unsigned int axisId, float value, float lastValue, double timestamp, void* context) {
  SystemImpl* obj = static_cast<SystemImpl*>(context);
  Device* dev = &obj->devices_[device];
  dev->axes[axisId] = value;
  if (obj->axis_move_handler_) {
    obj->axis_move_handler_(dev, axisId, value, lastValue, timestamp);
  }
}

}  // namespace gamepad

#endif  // _WIN32 or __APPLE__
