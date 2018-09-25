#ifndef GAMEPAD_OSX_HEADER
#define GAMEPAD_OSX_HEADER
#ifdef __APPLE__

#include <vector>
#include <IOKit/hid/IOHIDManager.h>

#include "gamepad.h"

namespace gamepad {

class SystemImpl;

struct HidButtonInfo {
  int button_id = -1;
  int cookie = -1;
};

struct HidAxisInfo {
  int axis_id = -1;
  int cookie = -1;
  float minimum = 0.0f;
  float maximum = 0.0f;
  float last_value = 0.0f;
};

struct HidDevice {
  IOHIDDeviceRef device_ref = nullptr;
  bool disconnected = false;
  Device device;
  std::vector<HidButtonInfo> button_map;
  std::vector<HidAxisInfo> axis_map;
};

class SystemImpl : public System {
 public:
  SystemImpl() = default;
  ~SystemImpl() override;
  void ProcessEvents() override;

 private:
  void HidInitialize();
  void HidCleanup(HidDevice* device);
  void HidReadInputs();
  void HidDeviceAttached(IOHIDDeviceRef device);
  void HidDeviceDetached(IOHIDDeviceRef device);
  void HidDeviceInput(IOHIDValueRef value);
  void HandleAxisEvent(HidDevice* device, int axis_id, int int_value);

  static void HidAttached(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static void HidDetached(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static void HidInput(void* context, IOReturn result, void* sender, IOHIDValueRef value);

 private:
  bool initialized_ = false;
  IOHIDManagerRef hid_manager_ = nullptr;
  std::vector<HidDevice> devices_;
};

}  // namespace gamepad

#endif  // __APPLE__
#endif  // GAMEPAD_OSX_HEADER