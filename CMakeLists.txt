cmake_minimum_required(VERSION 3.0)

project(OwlNM)
SET (OwlNM_VERSION_MAJOR 1)
SET (OwlNM_VERSION_MINOR 0)

SET (CMAKE_C_COMPILER             "clang")

IF("${CMAKE_SYSTEM}" MATCHES "Linux")
SET (CMAKE_C_FLAGS                "-Wall -std=gnu99")
ELSE()
SET (CMAKE_C_FLAGS                "-Wall -std=c99")
ENDIF()

SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "clang++")
SET (CMAKE_CXX_FLAGS                "-Wall")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(/usr/include)
INCLUDE_DIRECTORIES(/usr/local/include)

INCLUDE (CheckIncludeFiles)

CHECK_INCLUDE_FILES(wiringPi.h HAVE_WIRINGPI)
CHECK_INCLUDE_FILES("avahi-client/client.h;avahi-common/error.h" HAVE_AVAHI)

find_library(EVENT_LIBRARY
  NAMES event event2
  PATHS /usr/lib /usr/local/lib
)

CHECK_INCLUDE_FILES(event2/event.h HAVE_LIBEVENT)
CHECK_INCLUDE_FILES(/usr/src/sys/dev/evdev/input.h HAVE_BSD_INPUT)
CHECK_INCLUDE_FILES(linux/input.h HAVE_LINUX_INPUT)
CHECK_INCLUDE_FILES(input.h HAVE_INPUT)

LINK_DIRECTORIES(/usr/local/lib)

IF("${CMAKE_SYSTEM}" MATCHES "bsd")
INCLUDE_DIRECTORIES(/usr/src/sys/dev/evdev/)
ENDIF()

FILE(GLOB SOURCES "src/*.c")

ADD_EXECUTABLE(OwlNM ${SOURCES})

if(HAVE_WIRINGPI)
TARGET_LINK_LIBRARIES(OwlNM wiringPi event avahi-common avahi-client)
ELSE()
TARGET_LINK_LIBRARIES(OwlNM  event avahi-common avahi-client)
endif()

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h)

INSTALL(TARGETS OwlNM DESTINATION /usr/local/bin COMPONENT binaries)

install(FILES include/OwlNM.conf DESTINATION /usr/local/etc/ COMPONENT config)
