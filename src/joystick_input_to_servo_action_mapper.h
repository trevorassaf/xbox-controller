#ifndef XBOXCONTROLLER_JOYSTICKINPUTTOSERVOACTIONMAPPER_H
#define XBOXCONTROLLER_JOYSTICKINPUTTOSERVOACTIONMAPPER_H

#include <chrono>

#include "dynamixel/AxA12.h"

namespace xbox
{

class JoystickInputToServoActionMapper
{
public:
  static bool Create(dynamixel::AxA12 *servo, JoystickInputToServoActionMapper *out_mapper);

public:
  JoystickInputToServoActionMapper();
  JoystickInputToServoActionMapper(
      dynamixel::AxA12 *servo,
      bool inverted=false);
  JoystickInputToServoActionMapper(JoystickInputToServoActionMapper &&other);
  JoystickInputToServoActionMapper& operator=(JoystickInputToServoActionMapper &&other);
  bool ProcessInput(double value);

private:
  bool StopServoMovement();
  void StealResources(JoystickInputToServoActionMapper *other);

private:
  JoystickInputToServoActionMapper(const JoystickInputToServoActionMapper &other) = delete;
  JoystickInputToServoActionMapper& operator=(
      const JoystickInputToServoActionMapper &other) = delete;

private:
  bool initialized_;
  dynamixel::AxA12 *servo_;
  bool inverted_;
  std::chrono::steady_clock::time_point lockout_timepoint_;
  size_t movement_speed_;
  bool is_positive_movement_direction_;
  uint16_t stopped_position_;
};

}  // namespace xbox

#endif  // XBOXCONTROLLER_JOYSTICKINPUTTOSERVOACTIONMAPPER_H
