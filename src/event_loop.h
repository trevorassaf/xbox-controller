#ifndef XBOXCONTROLLER_EVENTLOOP_H
#define XBOXCONTROLLER_EVENTLOOP_H

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

#include <unordered_map>

#include "src/event_handler.h"

namespace xbox
{

class EventLoop
{
  public:
    static bool Create(EventLoop* out_loop);

  public:
    EventLoop();
    EventLoop(int fd);
    EventLoop(EventLoop&& other);
    EventLoop& operator=(EventLoop&& other);
    bool Add(EventHandler *handler);
    bool Run();

  private:
    void CloseResources();
    void StealResources(EventLoop* other);

  private:
    EventLoop(const EventLoop& other) = delete;
    EventLoop& operator=(const EventLoop& other) = delete;

  private:
    bool initialized_;
    int fd_;
};

}  // namespace xbox

#endif // XBOXCONTROLLER_EVENTLOOP_H
