#ifndef XBOXCONTROLLER_CONTROLLERPACKETTOPANTILTACTIONMAPPER_H
#define XBOXCONTROLLER_CONTROLLERPACKETTOPANTILTACTIONMAPPER_H

#include <chrono>
#include <cstdint>

#include "dynamixel/AxA12.h"
#include "src/joystick_input_to_servo_action_mapper.h"

namespace xbox
{

class ControllerPacketToPanTiltActionMapper
{
public:
  static bool Create(
      dynamixel::AxA12 *axa12_tilt,
      dynamixel::AxA12 *axa12_pan,
      ControllerPacketToPanTiltActionMapper *out_mapper);

public:
  ControllerPacketToPanTiltActionMapper();
  ControllerPacketToPanTiltActionMapper(
      JoystickInputToServoActionMapper tilt_action_mapper,
      JoystickInputToServoActionMapper pan_action_mapper);
  ControllerPacketToPanTiltActionMapper(ControllerPacketToPanTiltActionMapper &&other);
  ControllerPacketToPanTiltActionMapper& operator=(ControllerPacketToPanTiltActionMapper &&other);
  void ProcessPacket(
      const uint8_t *buffer,
      size_t buffer_size);

private:
  void StealResources(ControllerPacketToPanTiltActionMapper *other);

private:
  ControllerPacketToPanTiltActionMapper(
      const ControllerPacketToPanTiltActionMapper &other) = delete;
  ControllerPacketToPanTiltActionMapper& operator=(
      const ControllerPacketToPanTiltActionMapper &other) = delete;

private:
  bool initialized_;
  JoystickInputToServoActionMapper tilt_action_mapper_;
  JoystickInputToServoActionMapper pan_action_mapper_;
};

}  // namespace xbox

#endif  // XBOXCONTROLLER_CONTROLLERPACKETTOPANTILTACTIONMAPPER_H

