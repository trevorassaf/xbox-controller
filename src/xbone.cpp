#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Gpio/OutputPin.h"
#include "Gpio/RpiPinManager.h"
#include "Uart/UartClient.h"
#include "Uart/UartClientFactory.h"
#include "Uart/RpiUartContext.h"
#include "System/RpiSystemContext.h"

#include "dynamixel/AxA12.h"
#include "dynamixel/AxA12Factory.h"

#include "src/bluetooth_channel.h"
#include "src/controller_manager.h"
#include "src/controller_packet_to_pan_tilt_action_mapper.h"
#include "src/event_loop.h"

namespace
{
using dynamixel::AxA12;
using dynamixel::AxA12Factory;
using Gpio::OutputPin;
using Gpio::RpiPinManager;
using Uart::RpiUartContext;
using Uart::UartClient;
using System::RpiSystemContext;

const std::string XBOX_CONTROLLER_ADDRESS_2 = "C8:3F:26:11:D7:D3";
const std::string XBOX_CONTROLLER_ADDRESS_1 = "C8:3F:26:08:94:3F";
constexpr int GPIO_PIN_INDEX = 17;

void MapXbonePacketToServoAction(
    xbox::ControllerPacketToPanTiltActionMapper* mapper,
    const uint8_t* buffer,
    size_t buffer_length)
{
  assert(mapper);
  assert(buffer);
  mapper->ProcessPacket(buffer, buffer_length); 
}

}  // namespace

int main(int argc, char** argv)
{
  xbox::ControllerManager manager;
  std::vector<std::string> addresses;

  std::cout << "Searching for pairable XBox controllers..." << std::endl;

  if (!manager.FindPairableDevices(&addresses))
  {
    std::cerr << "Failed to find pairable xbox controllers" << std::endl;
  }

  if (addresses.empty())
  {
    std::cout << "No XBox controllers found! Attempting to connect "
              << XBOX_CONTROLLER_ADDRESS_1 << std::endl;

    if (!manager.Connect(XBOX_CONTROLLER_ADDRESS_1))
    {
      std::cerr << "Failed to pair!" << std::endl;
    }
    else
    {
      std::cout << "Successfully paired!" << std::endl;
    }
  }
  else
  {
    for (const std::string& addr : addresses)
    {
      std::cout << "XBox Controller Bluetooth Address: "
                <<  addr << std::endl;

      std::cout << "Attempting to pair..." << std::endl;
      if (!manager.Connect(addr))
      {
        std::cerr << "Failed to pair!" << std::endl;
      }
      else
      {
        std::cout << "Successfully paired!" << std::endl;
      }
    }
  }

  RpiSystemContext system_context;
  RpiPinManager gpio_manager;
  AxA12Factory axa12_factory;
  if (!AxA12Factory::Create(
            &system_context,
            &gpio_manager,
            GPIO_PIN_INDEX,
            &axa12_factory))
  {
    std::cerr << "Failed to create AxA12Factory" << std::endl;
    return EXIT_FAILURE;
  }
  AxA12 *axa12_tilt;
  uint8_t id_tilt = 1;
  if (!axa12_factory.Get(id_tilt, &axa12_tilt))
  {
    std::cerr << "Failed to initialize AxA12 servo" << std::endl;
    return false;
  }

  AxA12 *axa12_pan;
  uint8_t id_pan = 2;
  if (!axa12_factory.Get(id_pan, &axa12_pan))
  {
    std::cerr << "Failed to initialize AxA12 servo" << std::endl;
    return false;
  }

  xbox::ControllerPacketToPanTiltActionMapper pan_tilt_action_mapper;
  if (!xbox::ControllerPacketToPanTiltActionMapper::Create(
            axa12_tilt,
            axa12_pan,
            &pan_tilt_action_mapper))
  {
    std::cerr << "Failed to initialize controller-servo mapper" << std::endl;
    return EXIT_FAILURE;
  }

  xbox::BluetoothChannel bluetooth_channel;
  if (!xbox::BluetoothChannel::Create(
          [&] (const uint8_t *buffer, size_t length) {
              MapXbonePacketToServoAction(
                  &pan_tilt_action_mapper,
                  buffer,
                  length);
          },
          &bluetooth_channel))
  {
    std::cerr << "Failed to initialize BluetoothChannel" << std::endl;
    return EXIT_FAILURE;
  }

  xbox::EventLoop event_loop;
  if (!xbox::EventLoop::Create(&event_loop))
  {
    std::cerr << "Failed to initialize EventLoop" << std::endl;
    return EXIT_FAILURE;
  }

  if (!event_loop.Add(&bluetooth_channel))
  {
    std::cerr << "Failed to add BluetoothChannel to EventLoop" << std::endl;
    return EXIT_FAILURE;
  }

  if (!event_loop.Run())
  {
    std::cerr << "Failed to run EventLoop" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
