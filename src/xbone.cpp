#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "src/controller_manager.h"

int main(int argc, char** argv)
{
  xbox::ControllerManager manager;

  std::vector<std::string> addresses;

  std::cout << "Searching for pairable XBox controllers..." << std::endl;

  if (!manager.FindPairableDevices(&addresses))
  {
    std::cerr << "Failed to find pairable xbox controllers" << std::endl;
    return EXIT_FAILURE;
  }

  if (addresses.empty())
  {
    std::cout << "No XBox controllers found!" << std::endl;
    return EXIT_FAILURE;
  }

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

  return EXIT_SUCCESS;
}
