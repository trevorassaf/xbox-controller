#include "src/bluetooth_channel.h"

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>

#include <cassert>
#include <iostream>

#include <bluetooth/bluetooth.h>
#include <hci.h>

namespace
{
constexpr int INVALID_FD = -1;

struct mgmt_hdr
{
  uint16_t opcode;
  uint16_t index;
  uint16_t len;
} __packed;

} // namespace

namespace xbox
{

bool BluetoothChannel::Create(
    PacketCallback &&callback,
    BluetoothChannel* out_channel)
{
  assert(out_channel);

  int fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
  if (fd < 0)
  {
    std::cerr << "Failed to open bluetooth socket. Error: " << strerror(errno) << std::endl;
    return false;
  }

  struct sockaddr_hci address;
  memset(&address, 0, sizeof(address));
  address.hci_family = AF_BLUETOOTH;
  address.hci_dev = HCI_DEV_NONE;
  address.hci_channel = HCI_CHANNEL_MONITOR;

  if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    std::cerr << "Failed to bind socket. Error: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }

  *out_channel = BluetoothChannel{
      fd,
      std::move(callback)};
  return true;
}

BluetoothChannel::BluetoothChannel() : initialized_{false} {}

BluetoothChannel::BluetoothChannel(
    int fd,
    PacketCallback &&callback)
  : initialized_{true},
    fd_{fd},
    callback_{std::move(callback)} {}

BluetoothChannel::BluetoothChannel(BluetoothChannel&& other)
{
  StealResources(&other);
}

BluetoothChannel& BluetoothChannel::operator=(BluetoothChannel&& other)
{
  if (&other != this)
  {
    Close();
    StealResources(&other);
  }

  return *this;
}

int BluetoothChannel::GetFd() const
{
  assert(initialized_);
  return fd_;
}

void BluetoothChannel::HandlePacket()
{
  assert(initialized_);

  struct mgmt_hdr header;
  struct msghdr msg;
  struct iovec iov[2];
  uint8_t data[1490];
  uint8_t control[64];

  iov[0].iov_base = &header;
  iov[0].iov_len = sizeof(header);
  iov[1].iov_base = data;
  iov[1].iov_len = sizeof(data);

  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  int result = recvmsg(fd_, &msg, MSG_DONTWAIT);
  if (result < 0)
  {
    std::cerr << "Failed to read msg: " << strerror(errno) << std::endl;
    return;
  }

  int data_len = result - sizeof(header);
  callback_(data, data_len);
}

void BluetoothChannel::Close()
{
  if (!initialized_)
  {
    return;
  }

  close(fd_);
  fd_ = INVALID_FD;
  initialized_ = false;
}

void BluetoothChannel::StealResources(BluetoothChannel* other)
{
  assert(other);

  initialized_ = other->initialized_;
  other->initialized_ = false;
  fd_ = other->fd_;
  other->fd_ = INVALID_FD;
  callback_ = std::move(other->callback_);
}

}  // namespace xbox
