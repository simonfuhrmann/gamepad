#ifndef GAMEPAD_HEADER
#define GAMEPAD_HEADER

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gamepad {

struct Device {
  unsigned int device_id = 0;
  int vendor_id = 0;
  int product_id = 0;
  std::string description;
  std::vector<float> axes;
  std::vector<bool> buttons;
};

class System {
 public:
  // The attached handler signature.
  typedef std::function<void(Device*)> AttachedHandler;
  // The detached handler signature.
  typedef std::function<void(Device*)> DetachedHandler;
  // The button handler signature (device, button ID, timestamp).
  typedef std::function<void(Device*, unsigned int, double)> ButtonHandler;
  // The axis handler signature (device, axis ID, value, old value, timestamp).
  typedef std::function<void(Device*, unsigned int, float, float, double)> AxisHandler;

 public:
  static std::unique_ptr<System> Create();
  virtual ~System() = default;

  // Registers a handler that is called when a pad is attached.
  void RegisterAttachHandler(AttachedHandler handler);
  // Registers a handler that is called when a pad is detached.
  void RegisterDetachHandler(DetachedHandler handler);
  // Registers a handler for button down events.
  void RegisterButtonDownHandler(ButtonHandler handler);
  // Registers a handler for button up events.
  void RegisterButtonUpHandler(ButtonHandler handler);
  // Registers a handler for axis move events.
  void RegisterAxisMoveHandler(AxisHandler handler);

  // Processes all gamepad events and invokes the corresponding handler
  // functions.
  virtual void ProcessEvents() = 0;

 protected:
  System() = default;
  void HandleButtonEvent(Device* device, int button_id, int value);

  AttachedHandler attached_handler_;
  DetachedHandler detached_handler_;
  ButtonHandler button_up_handler_;
  ButtonHandler button_down_handler_;
  AxisHandler axis_move_handler_;
};

}  // namespace pad

#endif  // GAMEPAD_HEADER
