#include "src/event_loop.h"

#include <signal.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

namespace
{
constexpr int INVALID_FD = -1;
}  // namespace

namespace xbox
{

bool EventLoop::Create(EventLoop* out_loop)
{
  assert(out_loop);
  int fd = epoll_create1(EPOLL_CLOEXEC);

  if (fd < 0)
  {
    std::cerr << "Failed to initialize epoll. Error: " << strerror(errno) << std::endl;
    return false;
  }

  *out_loop = EventLoop{fd};
  return true;
}

EventLoop::EventLoop() : initialized_{false} {}

EventLoop::EventLoop(int fd)
  : initialized_{true},
    fd_{fd} {}

EventLoop::EventLoop(EventLoop&& other)
{
  StealResources(&other);
}

EventLoop& EventLoop::operator=(EventLoop&& other)
{
  if (this != &other)
  {
    CloseResources();
    StealResources(&other);
  }
  return *this;
}

bool EventLoop::Add(EventHandler *handler)
{
  assert(handler);
  assert(initialized_);

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.ptr = handler;

  if (epoll_ctl(fd_, EPOLL_CTL_ADD, handler->GetFd(), &ev) < 0)
  {
    std::cerr << "Failed to add epoll event. Error: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

bool EventLoop::Run()
{
  assert(initialized_);

  while (true)
  {
    struct epoll_event events[2];
    int active_fds = epoll_wait(fd_, events, 2, -1);

    if (active_fds < 0)
    {
      std::cerr << "Epoll active fd count less than zero. Error: " << strerror(errno)
                << std::endl;
      return false;
    }

    for (size_t i = 0; i < active_fds; ++i)
    {
      EventHandler *handler = (EventHandler *)events[i].data.ptr;
      handler->HandlePacket();
    }
  }

  return true;
}

void EventLoop::CloseResources()
{
  if (!initialized_)
  {
    return;
  }

  close(fd_);
  fd_ = INVALID_FD;
  initialized_ = false;
}

void EventLoop::StealResources(EventLoop* other)
{
  assert(other);

  initialized_ = other->initialized_;
  other->initialized_ = false;
  fd_ = other->fd_;
  other->fd_ = INVALID_FD;
}

}  // namespace xbox
