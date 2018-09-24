#ifndef GAMEPAD_LINUX_HEADER
#define GAMEPAD_LINUX_HEADER
#ifdef __linux__

#include <libevdev/libevdev.h>
#include <string>

#include "gamepad.h"

namespace gamepad {

struct EvdevKeyInfo {
  int button_id = -1;
};

struct EvdevAxisInfo {
  int axis_id = -1;
  float minimum = 0.0f;
  float maximum = 0.0f;
  float last_value = 0.0f;
  int flat = 0;
};

struct EvdevDevice {
  std::string filename;
  int file_descriptor = -1;
  struct libevdev* evdev = nullptr;
  Device device;
  std::vector<EvdevKeyInfo> key_map;
  std::vector<EvdevAxisInfo> axis_map;
};

class SystemImpl : public System {
 public:
  SystemImpl() = default;
  virtual ~SystemImpl();

  void ProcessEvents();

 private:
  void Initialize();
  void EvdevCleanup(EvdevDevice* device);
  void EvdevInitialize(const std::string& filename);
  void EvdevReadInputs();
  void EvdevProcessEvent(EvdevDevice* device, const struct input_event& event);

 private:
  bool initialized_ = false;
  std::vector<EvdevDevice> devices_;
};

}  // namespace gamepad

#endif  // __linux__
#endif  // GAMEPAD_LINUX_HEADER
