cmake_minimum_required(VERSION 3.0)

project(OwlNM)
SET (OwlNM_VERSION_MAJOR 0)
SET (OwlNM_VERSION_MINOR 8)

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

LINK_DIRECTORIES(/usr/local/lib)

IF("${CMAKE_SYSTEM}" MATCHES "bsd")
INCLUDE_DIRECTORIES(/usr/src/sys/dev/evdev/)
ENDIF()

FILE(GLOB SOURCES "src/*.c")

ADD_EXECUTABLE(OwlNM ${SOURCES})

TARGET_LINK_LIBRARIES(OwlNM  event avahi-common avahi-client)

INSTALL(TARGETS OwlNM DESTINATION /usr/local/bin COMPONENT binaries)
install(FILES include/OwlNM.conf DESTINATION /usr/local/etc/ COMPONENT config)
