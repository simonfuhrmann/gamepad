#ifndef GAMEPAD_LIBSTEM_HEADER
#define GAMEPAD_LIBSTEM_HEADER
#if defined(_WIN32) || defined(__APPLE__)

#include <map>

#include "gamepad.h"
#include "libstem_gamepad/source/gamepad/Gamepad.h"

namespace gamepad {

class SystemImpl : public System {
 public:
  SystemImpl() = default;
  void ProcessEvents();

 protected:
  static void OnAttached(Gamepad_device* device, void* context);
  static void OnDetached(Gamepad_device* device, void* context);
  static void OnButtonDown(Gamepad_device* device, unsigned int buttonId, double timestamp, void* context);
  static void OnButtonUp(Gamepad_device* device, unsigned int buttonId, double timestamp, void* context);
  static void OnAxisMove(Gamepad_device* device, unsigned int axisId, float value, float lastValue, double timestamp, void* context);

 private:
  void CreateDevice(Gamepad_device* device);

 private:
  bool initialized_ = false;
  std::map<Gamepad_device*, Device> devices_;
};

}  // namespace gamepad

#endif  // _WIN32 or __APPLE__
#endif  // GAMEPAD_LIBSTEM_HEADER
