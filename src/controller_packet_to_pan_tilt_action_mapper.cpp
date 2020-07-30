#include "src/controller_packet_to_pan_tilt_action_mapper.h"

#include <cassert>
#include <iostream>

namespace
{
using dynamixel::AxA12;

constexpr size_t MINIMUM_PACKET_SIZE = 25;

double NormalizeJoystickInputScalar(uint16_t joystick_input_scalar)
{
  uint64_t joystick_input_scalar_long = static_cast<uint64_t>(joystick_input_scalar);
  int64_t signed_scalar_long = joystick_input_scalar_long - 0x8000;
  return 2.0 * signed_scalar_long / 0x10000;
}

void PrintPacket(const uint8_t *buffer, size_t length)
{
  assert(buffer);

  std::cout << "Bluetooth packet dump..." << std::endl;

  for (size_t i = 0; i < length; ++i)
  {
    std::cout << "buffer[" << i << "]=0x" << std::hex << (int)buffer[i]
              << std::dec << std::endl;
  }
}

}  // namespace

namespace xbox
{

bool ControllerPacketToPanTiltActionMapper::Create(
    AxA12 *axa12_tilt,
    AxA12 *axa12_pan,
    ControllerPacketToPanTiltActionMapper *out_mapper)
{
  assert(axa12_tilt);
  assert(axa12_pan);
  assert(out_mapper);

  JoystickInputToServoActionMapper tilt_action_mapper;
  if (!JoystickInputToServoActionMapper::Create(axa12_tilt, &tilt_action_mapper))
  {
    std::cerr << "Failed to initialize tilt servo joystick mapper" << std::endl;
    return false;
  }

  JoystickInputToServoActionMapper pan_action_mapper;
  if (!JoystickInputToServoActionMapper::Create(axa12_pan, &pan_action_mapper))
  {
    std::cerr << "Failed to initialize pan servo joystick mapper" << std::endl;
    return false;
  }

  *out_mapper = ControllerPacketToPanTiltActionMapper{
        std::move(tilt_action_mapper),
        std::move(pan_action_mapper)};
  return true;
}

ControllerPacketToPanTiltActionMapper::ControllerPacketToPanTiltActionMapper()
  : initialized_{false} {}

ControllerPacketToPanTiltActionMapper::ControllerPacketToPanTiltActionMapper(
    JoystickInputToServoActionMapper tilt_action_mapper,
    JoystickInputToServoActionMapper pan_action_mapper)
  : initialized_{true},
    tilt_action_mapper_{std::move(tilt_action_mapper)},
    pan_action_mapper_{std::move(pan_action_mapper)} {}

ControllerPacketToPanTiltActionMapper::ControllerPacketToPanTiltActionMapper(
    ControllerPacketToPanTiltActionMapper &&other)
{
  StealResources(&other);
}

ControllerPacketToPanTiltActionMapper& ControllerPacketToPanTiltActionMapper::operator=(
    ControllerPacketToPanTiltActionMapper &&other)
{
  if (this != &other)
  {
    StealResources(&other);
  }
  return *this;
}

void ControllerPacketToPanTiltActionMapper::ProcessPacket(
    const uint8_t *buffer,
    size_t buffer_size)
{
  assert(initialized_);
  assert(buffer);

  if (buffer_size < MINIMUM_PACKET_SIZE)
  {
    std::cerr << "Rejecting packet. Expected length " << MINIMUM_PACKET_SIZE
              << " but received actual packet size of " << buffer_size << std::endl;
    return;
  }

  uint16_t tilt_position = (buffer[13] << 8) | buffer[12];
  double normalized_tilt_position = NormalizeJoystickInputScalar(tilt_position);

  uint16_t pan_position = (buffer[11] << 8) | buffer[10];
  double normalized_pan_position = NormalizeJoystickInputScalar(pan_position);

  if (!tilt_action_mapper_.ProcessInput(normalized_tilt_position))
  {
    std::cerr << "Failed to process tilt joystick value" << std::endl;
    return;
  }

  if (!pan_action_mapper_.ProcessInput(normalized_pan_position))
  {
    std::cerr << "Failed to process pan joystick value" << std::endl;
    return;
  }
}

void ControllerPacketToPanTiltActionMapper::StealResources(
    ControllerPacketToPanTiltActionMapper *other)
{
  assert(other);
  initialized_ = other->initialized_;
  other->initialized_ = false;
  tilt_action_mapper_ = std::move(other->tilt_action_mapper_);
  pan_action_mapper_ = std::move(other->pan_action_mapper_);
}

}  // namespace xbox
