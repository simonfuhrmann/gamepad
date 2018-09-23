
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

#include "libstem_gamepad/source/gamepad/Gamepad.h"

void PrintDeviceState(struct Gamepad_device* device) {
  std::cout << "Buttons: ";
  for (unsigned int i = 0; i < device->numButtons; ++i) {
    std::cout << device->buttonStates[i] << " ";
  }
  std::cout << "Axes: ";
  for (unsigned int i = 0; i < device->numAxes; ++i) {
    const double value = device->axisStates[i];
    const int print = static_cast<int>(value * 100.0);
    std::cout << std::setw(4) << print << " ";
  }
  std::cout << std::endl;
}

void ButtonFunc(struct Gamepad_device* device, unsigned int buttonID, double timestamp, void * context) {
  PrintDeviceState(device);
}

void AxisFunc(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context) {
  PrintDeviceState(device);
}

void AttachFunc(struct Gamepad_device * device, void * context) {
  std::cout << "Attached: " << device->description << std::endl;
  std::cout << "  id=" << device->deviceID << " vendor=" << device->vendorID
      << " product=" << device->productID << std::endl;
  std::cout << "  buttons=" << device->numButtons
      << " axis=" << device->numAxes << std::endl;
}

void DetachFunc(struct Gamepad_device * device, void * context) {
  std::cout << "Detached ID " << device->deviceID << std::endl;
}

int main(void) {
  Gamepad_deviceAttachFunc(AttachFunc, nullptr);
  Gamepad_deviceRemoveFunc(DetachFunc, nullptr);
  Gamepad_buttonDownFunc(ButtonFunc, nullptr);
  Gamepad_buttonUpFunc(ButtonFunc, nullptr);
  Gamepad_axisMoveFunc(AxisFunc, nullptr);
  Gamepad_detectDevices();
  Gamepad_init();

  while (true) {
    Gamepad_processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}
