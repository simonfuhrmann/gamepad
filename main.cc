#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

#include "gamepad.h"

void device_attached(gamepad::Device* device) {
  std::cout << "Attached: " << device->description << std::endl;
  std::cout << "  id=" << device->device_id << " vendor=" << device->vendor_id
      << " product=" << device->product_id << std::endl;
  std::cout << "  buttons=" << device->buttons.size()
      << " axis=" << device->axes.size() << std::endl;
}

void device_detached(gamepad::Device* device) {
  std::cout << "Detached ID " << device->device_id << std::endl;
}

void print_device_state(gamepad::Device* device) {
  std::cout << "Buttons: ";
  for (unsigned int i = 0; i < device->buttons.size(); ++i) {
    std::cout << device->buttons[i] << " ";
  }
  std::cout << "Axes: ";
  for (unsigned int i = 0; i < device->axes.size(); ++i) {
    const float value = device->axes[i];
    const int print = static_cast<int>(value * 100.0);
    std::cout << std::setw(4) << print << " ";
  }
  std::cout << std::endl;
}

void button_event(gamepad::Device* device, unsigned int, double) {
  print_device_state(device);
}

void axis_event(gamepad::Device* device, unsigned int, float, float, double) {
  print_device_state(device);
}

int main(int argc, char** argv) {
  std::unique_ptr<gamepad::System> gamepad = gamepad::System::Create();
  gamepad->RegisterAttachHandler(device_attached);
  gamepad->RegisterDetachHandler(device_detached);
  gamepad->RegisterButtonUpHandler(button_event);
  gamepad->RegisterButtonDownHandler(button_event);
  gamepad->RegisterAxisMoveHandler(axis_event);

  while (true) {
    gamepad->ProcessEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}
