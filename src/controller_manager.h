#ifndef XBOXCONTROLLER_CONTROLLERMANAGER_H
#define XBOXCONTROLLER_CONTROLLERMANAGER_H

#include <string>
#include <vector>

namespace xbox
{

class ControllerManager
{
public:
  bool FindPairableDevices(std::vector<std::string> *out_addresses);
  bool Connect(const std::string& addr);
};

}  // namespace xbox

#endif  // XBOXCONTROLLER_CONTROLLERMANAGER_H
