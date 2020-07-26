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
#include <glib.h>
#include <gio/gio.h>

namespace
{
constexpr const char* XBOX_CONTROLLER_NAME = "Xbox Wireless Controller";

// XBOX target bluez name "/org/bluez/hci0/dev_C8_3F_26_11_D7_D3",
constexpr const char* XBOX_CONTROLLER_BLUEZ_PREFIX = "/org/bluez/hci0/dev_";
constexpr int XBOX_PAIRING_TIMEOUT_SECS = 3;

std::string ToBluezDevicePath(const std::string& address)
{
  std::string altered_address = address;
  for (size_t i = 0; i < altered_address.size(); ++i)
  {
    if (altered_address.at(i) == ':')
    {
      altered_address.at(i) = '_';
    }
  }

  return std::string{XBOX_CONTROLLER_BLUEZ_PREFIX} + altered_address;
}

gboolean shutdown_pairing_loop(gpointer data)
{
  g_main_loop_quit((GMainLoop*)data);
  return FALSE;
}

int bluez_adapter_call_method(GDBusConnection *con, const char *method)
{
	GVariant *result = nullptr;
	GError *error = nullptr;

	result = g_dbus_connection_call_sync(con,
					     "org.bluez",
					/* TODO Find the adapter path runtime */
					     "/org/bluez/hci0",
					     "org.bluez.Adapter1",
					     method,
					     nullptr,
					     nullptr,
					     G_DBUS_CALL_FLAGS_NONE,
					     -1,
					     nullptr,
					     &error);
	if (error)
  {
    std::cerr << "Failed to call bluez adapter method: "
              << error->message << std::endl;
		return false;
  }

	g_variant_unref(result);
	return true;
}

bool bluez_adapter_set_property(
    GDBusConnection* con,
    const char *prop,
    GVariant *value)
{
	GVariant *result;
	GError *error = nullptr;

	result = g_dbus_connection_call_sync(con,
					     "org.bluez",
					     "/org/bluez/hci0",
					     "org.freedesktop.DBus.Properties",
					     "Set",
					     g_variant_new("(ssv)", "org.bluez.Adapter1", prop, value),
					     nullptr,
					     G_DBUS_CALL_FLAGS_NONE,
					     -1,
					     nullptr,
					     &error);
	if (error)
  {
    std::cerr << "Failed to set bluez adapter property: "
              << error->message << std::endl;
    return false;
  }

	g_variant_unref(result);
  return true;
}

}  // namespace

namespace xbox
{

bool ControllerManager::FindPairableDevices(std::vector<std::string> *out_addresses)
{
  inquiry_info *ii = nullptr;
  int max_rsp, num_rsp;
  int dev_id = -1;
  int sock = -1;
  int len = -1;
  int flags = -1;
  int i;
  char addr[19] = {0};
  char name[248] = {0};
  bool succeeded = false;

  dev_id = hci_get_route(nullptr);
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
 
  std::cout << "bozkurtus -- FindPairableDevices() -- before hci_inquiry()" << std::endl; 

  num_rsp = hci_inquiry(dev_id, len, max_rsp, nullptr, &ii, flags);
  if (num_rsp < 0)
  {
    std::cerr << "Failed to perform hci_inquiry. Error: "
              <<  strerror(errno) << std::endl;
    goto done;
  }

  std::cout << "bozkurtus -- FindPairableDevices() -- after hci_inquiry()" << std::endl; 

  for (i = 0; i < num_rsp; i++) {
      ba2str(&(ii+i)->bdaddr, addr);
      memset(name, 0, sizeof(name));

  std::cout << "bozkurtus -- FindPairableDevices() -- before hci_read_remote_name()" << std::endl; 

      if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
          name, 0) < 0)
      {
        strcpy(name, "[unknown]");
      }

  std::cout << "bozkurtus -- FindPairableDevices() -- after hci_read_remote_name()" << std::endl; 

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

bool ControllerManager::Connect(const std::string& xbox_controller_address)
{
	int rc;
  GMainLoop *loop;
	GVariant *result;
	GError *error = nullptr;
  bool successful = false;

  std::string bluez_target_name =
      ToBluezDevicePath(xbox_controller_address);

  std::cout << "bozkurtus -- ControllerManager::Connect() -- bluez_target_name="
            << bluez_target_name << std::endl;


	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
	if(!connection) {
    std::cout << "Not able to get connection to system bus" << std::endl;
    goto done;
	}

	loop = g_main_loop_new(nullptr, FALSE);

	if (bluez_adapter_set_property(connection, "Powered", g_variant_new("b", TRUE)) < 0)
	{
    std::cerr << "Not able to enable the adapter" << std::endl;
		goto done;
	}

	if (bluez_adapter_call_method(connection, "StartDiscovery") < 0)
	{
    std::cerr << "Not able to scan for new devices" << std::endl;
		goto done;
	}

	g_dbus_connection_call_sync(connection,
				"org.bluez",
        bluez_target_name.c_str(),
				"org.bluez.Device1",
				"Connect",
				nullptr,
        nullptr,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				nullptr,
        &error);

  if (error)
  {
    std::cout << "Error! " << error->message << std::endl;
    goto done;
  }
  else
  {
    g_variant_unref(result);
  }

  g_timeout_add_seconds(XBOX_PAIRING_TIMEOUT_SECS, shutdown_pairing_loop, loop);
	g_main_loop_run(loop);

	if (bluez_adapter_call_method(connection, "StopDiscovery") < 0)
  {
    std::cerr << "Failed to stop discovery" << std::endl;
    goto done;
  }

  successful = true;

done:
  if (connection)
  {
    g_object_unref(connection);
  }
	return successful;
}

}  // namespace xbox
