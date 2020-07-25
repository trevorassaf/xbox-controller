#include "src/controller_manager.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <iostream>
#include <string>
#include <vector>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

namespace
{
constexpr const char* XBOX_CONTROLLER_NAME = "Xbox Wireless Controller";
}  // namespace

namespace xbox
{

bool ControllerManager::FindPairableDevices(std::vector<std::string> *out_addresses)
{
  inquiry_info *ii = NULL;
  int max_rsp, num_rsp;
  int dev_id = -1;
  int sock = -1;
  int len = -1;
  int flags = -1;
  int i;
  char addr[19] = {0};
  char name[248] = {0};
  bool succeeded = false;

  dev_id = hci_get_route(NULL);
  if (dev_id < 0)
  {
    std::cerr << "Failed to get hci route. Error: " << strerror(errno) << std::endl;
    goto done;
  }

  sock = hci_open_dev(dev_id);
  if (sock < 0)
  {
    std::cerr << "Failed to open hci socket: id=" << dev_id << ". Error="
              << strerror(errno) << std::endl;
    goto done;
  }

  len  = 8;
  max_rsp = 255;
  flags = IREQ_CACHE_FLUSH;
  ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
  
  num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
  if (num_rsp < 0)
  {
    std::cerr << "Failed to perform hci_inquiry. Error: "
              <<  strerror(errno) << std::endl;
    goto done;
  }

  for (i = 0; i < num_rsp; i++) {
      ba2str(&(ii+i)->bdaddr, addr);
      memset(name, 0, sizeof(name));
      if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
          name, 0) < 0)
      {
        strcpy(name, "[unknown]");
      }

      if (strncmp(XBOX_CONTROLLER_NAME, name, sizeof(XBOX_CONTROLLER_NAME)) == 0)
      {
        out_addresses->push_back(addr);
      }
  }

  succeeded = true;

done:
  if (ii)
  {
    free(ii);
  }

  if (sock >= 0)
  {
    close(sock);
  }

  return succeeded;
}

}  // namespace xbox
