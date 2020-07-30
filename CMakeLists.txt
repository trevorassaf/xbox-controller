cmake_minimum_required(VERSION 3.1)

include_directories(
  .
  ../../bluez-prebuilts/repo/include/bluez-5.50
  ../../bluez-prebuilts/repo/include/bluez-5.50/lib
  ../../bluez-prebuilts/repo/src/gdbus
  ../../glib-prebuilts/repo/include
  ../../glib-prebuilts/repo/include/glib
  ../../glib-prebuilts/repo/include/gio
  ../../glib-prebuilts/repo/include/gmodule
  ../../glib-prebuilts/repo/include/gobject
  ../../gpio14/repo/include
  ../../dbus-prebuilts/repo/include/dbus-1.12.20
  ../../sentry-client/repo/src)

file(GLOB BLUEZ_PREBUILT_LIBRARIES "../../bluez-prebuilts/repo/lib/*.so*")
message("BLUEZ_LIBRARIES = ${BLUEZ_PREBUILT_LIBRARIES}")

file(GLOB DBUS_PREBUILT_LIBRARIES "../../dbus-prebuilts/repo/lib/*.so*")
message("DBUS_LIBRARIES = ${DBUS_PREBUILT_LIBRARIES}")

file(GLOB GLIB_PREBUILT_LIBRARIES "../../glib-prebuilts/repo/lib/*.so*")
message("GLIB_LIBRARIES = ${GLIB_PREBUILT_LIBRARIES}")

add_executable(xbone
  src/xbone.cpp
  src/bluetooth_channel.cpp
  src/controller_manager.cpp
  src/controller_packet_to_pan_tilt_action_mapper.cpp
  src/event_loop.cpp
  src/joystick_input_to_servo_action_mapper.cpp)

target_link_libraries(xbone ${BLUEZ_PREBUILT_LIBRARIES})
target_link_libraries(xbone ${DBUS_PREBUILT_LIBRARIES})
target_link_libraries(xbone ${GLIB_PREBUILT_LIBRARIES})
target_link_libraries(xbone gpio14)
target_link_libraries(xbone dynamixel)
target_link_libraries(xbone gflags::gflags)
target_link_libraries(xbone glog::glog)
