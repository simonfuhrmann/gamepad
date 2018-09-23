#ifndef GAMEPAD_HEADER
#define GAMEPAD_HEADER

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gamepad {

struct Device {
  unsigned int device_id;
  int vendor_id;
  int product_id;
  std::string description;
  std::vector<float> axes;
  std::vector<bool> buttons;
};

class System {
 public:
  typedef std::function<void(Device*)> AttachedHandler;
  typedef std::function<void(Device*)> DetachedHandler;
  typedef std::function<void(Device*, unsigned int, double)> ButtonHandler;
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

  virtual void ProcessEvents() = 0;

 protected:
  System() = default;

  AttachedHandler attached_handler_;
  DetachedHandler detached_handler_;
  ButtonHandler button_up_handler_;
  ButtonHandler button_down_handler_;
  AxisHandler axis_move_handler_;
};

}  // namespace pad

#endif  // GAMEPAD_HEADER
