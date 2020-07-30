#ifndef XBOXCONTROLLER_EVENTHANDLER_H
#define XBOXCONTROLLER_EVENTHANDLER_H

namespace xbox
{
class EventHandler
{
  public:
    virtual int GetFd() const = 0;
    virtual void HandlePacket() = 0;
};
}  // namespace xbox

#endif // XBOXCONTROLLER_EVENTHANDLER_H
