#include "src/joystick_input_to_servo_action_mapper.h"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>

namespace
{
constexpr uint16_t GOAL_POSITION_LIMIT_LOW = 400;
constexpr uint16_t GOAL_POSITION_LIMIT_HIGH = 650;
constexpr uint16_t GOAL_POSITION_NEUTRAL = 512;

constexpr uint16_t STOP_MOVEMENT_GOAL_POSITION_LIMIT_LOW = 511;
constexpr uint16_t STOP_MOVEMENT_GOAL_POSITION_LIMIT_HIGH = 513;

constexpr uint16_t LOW_MOVEMENT_SPEED = 0x00F;
//constexpr uint16_t MAX_MOVEMENT_SPEED = 0x3FF;
constexpr uint16_t MAX_MOVEMENT_SPEED = 0x0FF;
const std::array<uint16_t, 3> MOVEMENT_SPEEDS =
{
  0,
  LOW_MOVEMENT_SPEED,
  MAX_MOVEMENT_SPEED,
};

constexpr uint16_t MAX_TORQUE = 0x3FF;

const std::chrono::milliseconds LOCKOUT_DELTA{10};

}  // namespace

namespace xbox
{
using dynamixel::AxA12;

bool JoystickInputToServoActionMapper::Create(
    AxA12 *servo,
    JoystickInputToServoActionMapper *out_mapper)
{
  assert(servo);
  assert(out_mapper);

  if (!servo->SetGoalPosition(GOAL_POSITION_NEUTRAL))
  {
    std::cerr << "Failed to initialize AxA12 to neutral position" << std::endl;
    return false;
  }

  if (!servo->SetClockWiseAngleLimit(GOAL_POSITION_LIMIT_LOW))
  {
    std::cerr << "Failed to set servo clockwise angle limit: 0x"
              << std::hex << GOAL_POSITION_LIMIT_LOW << std::dec << std::endl;
    return false;
  }

  if (!servo->SetCounterClockWiseAngleLimit(GOAL_POSITION_LIMIT_HIGH))
  {
    std::cerr << "Failed to set servo counter clockwise angle limit: 0x"
              << std::hex << GOAL_POSITION_LIMIT_HIGH << std::dec << std::endl;
    return false;
  }

  /*
  uint16_t initial_torque_limit;
  if (!servo->GetTorqueLimit(&initial_torque_limit))
  {
    std::cerr << "Failed to read initial torque value" << std::endl;
    return false;
  }

  std::cout << "bozkurtus -- JoystickInputToServoActionMapper::Create() -- initial_torque: 0x"
            << std::hex << initial_torque_limit << std::dec << std::endl;
  */

  if (!servo->SetTorqueLimit(MAX_TORQUE))
  {
    std::cerr << "Failed to initialize servo with max torque" << std::endl;
    return false;
  }

  *out_mapper = JoystickInputToServoActionMapper{servo};
  return true;
}

JoystickInputToServoActionMapper::JoystickInputToServoActionMapper()
  : initialized_{false} {}

JoystickInputToServoActionMapper::JoystickInputToServoActionMapper(
    AxA12 *servo,
    bool inverted)
  : initialized_{true},
    servo_{servo},
    inverted_{inverted},
    lockout_timepoint_{std::chrono::steady_clock::now()},
    movement_speed_{0},
    is_positive_movement_direction_{false},
    stopped_position_{GOAL_POSITION_NEUTRAL} {}

JoystickInputToServoActionMapper::JoystickInputToServoActionMapper(
    JoystickInputToServoActionMapper &&other)
{
  StealResources(&other);
}

JoystickInputToServoActionMapper& JoystickInputToServoActionMapper::operator=(
    JoystickInputToServoActionMapper &&other)
{
  if (this != &other)
  {
    StealResources(&other);
  }
  return *this;
}

bool JoystickInputToServoActionMapper::ProcessInput(double value)
{
  assert(initialized_);

  auto now = std::chrono::steady_clock::now();
  if (now < lockout_timepoint_)
  {
    return true;
  }

  assert(!MOVEMENT_SPEEDS.empty());
  size_t index = (std::abs(value) == 1)
      ? MOVEMENT_SPEEDS.size() - 1
      : std::abs(value) * MOVEMENT_SPEEDS.size();

  assert(index < MOVEMENT_SPEEDS.size());
  uint16_t target_speed = MOVEMENT_SPEEDS.at(index);

  if (target_speed == 0)
  {
    if (movement_speed_ != 0 && !StopServoMovement())
    {
      std::cerr << "Failed to stop servo movement. Value="
                << value << std::endl;
      return false;
    }

    lockout_timepoint_ = std::chrono::steady_clock::now() + LOCKOUT_DELTA;
    return true;
  }

  bool target_positive_direction = value > 0;
  if (target_positive_direction == is_positive_movement_direction_ &&
      movement_speed_ == target_speed)
  {
    return true;
  }

  /*
  if (!servo_->SetGoalPosition(stopped_position_))
  {
    std::cerr << "Failed to set goal position to current position before initiating movement"
              << std::endl;
    return false;
  }
  */

  bool previous_movement_speed = movement_speed_;
  if (target_speed != movement_speed_)
  {
    std::cerr << "bozkurtus -- JoystickInputToServoActionMapper::ProcessInput() -- "
              << "old_movement_speed=" << movement_speed_ << ", new_movement_speed="
              << target_speed << std::endl;

    if (!servo_->SetMovingSpeed(target_speed))
    {
      std::cerr << "Failed to set movement speed: " << target_speed << std::endl;
      return false;
    }

    movement_speed_ = target_speed;
  }

  /*
  if (previous_movement_speed != 0 &&
      target_positive_direction != is_positive_movement_direction_)
  {
  */
    assert(value != 0);
    uint16_t target_position = (target_positive_direction)
        ? GOAL_POSITION_LIMIT_HIGH
        : GOAL_POSITION_LIMIT_LOW;

    std::cerr << "bozkurtus -- JoystickInputToServoActionMapper::ProcessInput() -- new_direction="
              << (int)target_positive_direction << ", old_direction="
              << (int)is_positive_movement_direction_ << std::endl;

    if (!servo_->SetGoalPosition(target_position))
    {
      std::cerr << "Failed to set goal position: 0x" << std::hex
                << target_position << std::dec << std::endl;
      return false;
    }

    is_positive_movement_direction_ = target_positive_direction;
    /*
  }
  */

  if (!servo_->SetTorqueEnabled(true))
  {
    std::cerr << "Failed to re-enable torque" << std::endl;
    return false;
  }

  lockout_timepoint_ = std::chrono::steady_clock::now() + LOCKOUT_DELTA;
  return true;
}

bool JoystickInputToServoActionMapper::StopServoMovement()
{
  assert(movement_speed_ > 0);

  /*
  uint16_t current_position;
  if (!servo_->GetPresentPosition(&current_position))
  {
    std::cerr << "Failed to get current position" << std::endl;
    return false;
  }

  std::cerr << "bozkurtus -- JoystickInputToServoActionMapper::StopServoMovement() -- "
            << "current_position=" << current_position << ". Transitioning from: movement_speed_="
            << movement_speed_ << ", direction=" << (int)is_positive_movement_direction_
            << std::endl;

  if (!servo_->SetGoalPosition(current_position))
  {
    std::cerr << "Failed to set current goal position: "
              << current_position << std::endl;
    return false;
  }
  */

  /*
  // Set CW and CCW limits to small sector. This stops the servo motion regardless of its
  // current position
  if (!servo_->SetClockWiseAngleLimit(STOP_MOVEMENT_GOAL_POSITION_LIMIT_LOW))
  {
    std::cerr << "Failed to set stop motion servo clockwise angle limit: 0x"
              << std::hex << STOP_MOVEMENT_GOAL_POSITION_LIMIT_LOW << std::dec << std::endl;
    return false;
  }

  if (!servo_->SetCounterClockWiseAngleLimit(STOP_MOVEMENT_GOAL_POSITION_LIMIT_HIGH))
  {
    std::cerr << "Failed to set stop motion servo counter clockwise angle limit: 0x"
              << std::hex << STOP_MOVEMENT_GOAL_POSITION_LIMIT_HIGH << std::dec << std::endl;
    return false;
  }
  */

  std::cerr << "bozkurtus -- JoystickInputToServoActionMapper::StopServoMovement() -- call"
            << std::endl;
  if (!servo_->SetTorqueEnabled(false))
  {
    std::cerr << "Failed to disable torque in order to stop motion" << std::endl;
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds{5});

  /*
  uint16_t stopped_position_;
  if (!servo_->GetPresentPosition(&stopped_position_))
  {
    std::cerr << "Failed to get current position" << std::endl;
    return false;
  }

  std::cerr << "bozkurtus -- JoystickInputToServoActionMapper::StopServoMovement() -- "
            << "current_position=" << stopped_position_ << ". Transitioning from: movement_speed_="
            << movement_speed_ << ", direction=" << (int)is_positive_movement_direction_
            << std::endl;
  */

  movement_speed_ = 0;
  return true;
}

void JoystickInputToServoActionMapper::StealResources(JoystickInputToServoActionMapper *other)
{
  assert(other);

  initialized_ = other->initialized_;
  other->initialized_ = false;
  servo_ = other->servo_;
  other->servo_ = nullptr;
  inverted_ = other->inverted_;
  lockout_timepoint_ = std::move(other->lockout_timepoint_);
  movement_speed_ = other->movement_speed_;
  other->movement_speed_ = 0;
  is_positive_movement_direction_ = other->is_positive_movement_direction_;
  stopped_position_ = other->stopped_position_;
}

}  // namespace xbox
