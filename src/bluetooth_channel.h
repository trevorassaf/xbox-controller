#ifndef XBOXCONTROLLER_BLUETOOTHCHANNEL_H
#define XBOXCONTROLLER_BLUETOOTHCHANNEL_H

#include <cstdint>
#include <functional>

#include "src/event_handler.h"

namespace xbox
{

class BluetoothChannel : public EventHandler
{
public:
  using PacketCallback = std::function<void(const uint8_t *buffer, size_t length)>;

public:
  static bool Create(PacketCallback &&callback, BluetoothChannel* out_channel);

public:
  BluetoothChannel();
  BluetoothChannel(
      int fd,
      PacketCallback &&callback);
  BluetoothChannel(BluetoothChannel&& other);
  BluetoothChannel& operator=(BluetoothChannel&& other);
  int GetFd() const override;
  void HandlePacket() override;

private:
  void Close();
  void StealResources(BluetoothChannel* other);

private:
  BluetoothChannel(const BluetoothChannel& other) = delete;
  BluetoothChannel& operator=(const BluetoothChannel& other) = delete;

private:
  bool initialized_;
  int fd_;
  PacketCallback callback_;
};

}  // namespace xbox

#endif  // XBOXCONTROLLER_BLUETOOTHCHANNEL_H
