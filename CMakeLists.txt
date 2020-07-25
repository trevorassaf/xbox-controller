cmake_minimum_required(VERSION 3.1)

include_directories(
  ../../bluez-prebuilts/repo/include/bluez-5.50
  ../../bluez-prebuilts/repo/include/bluez-5.50/lib)

file(GLOB BLUEZ_PREBUILT_LIBRARIES "../../bluez-prebuilts/repo/lib/*.so*")
message("BLUEZ_LIBRARIES = ${BLUEZ_PREBUILT_LIBRARIES}")

file(GLOB DBUS_PREBUILT_LIBRARIES "../../dbus-prebuilts/repo/lib/*.so*")
message("DBUS_LIBRARIES = ${DBUS_PREBUILT_LIBRARIES}")

file(GLOB GLIB_PREBUILT_LIBRARIES "../../glib-prebuilts/repo/lib/*.so*")
message("GLIB_LIBRARIES = ${GLIB_PREBUILT_LIBRARIES}")

add_executable(xbone src/xbone.cpp)

target_link_libraries(xbone ${BLUEZ_PREBUILT_LIBRARIES})
target_link_libraries(xbone ${DBUS_PREBUILT_LIBRARIES})
target_link_libraries(xbone ${GLIB_PREBUILT_LIBRARIES})