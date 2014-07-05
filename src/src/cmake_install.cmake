# Install script for directory: /Users/jimbolaptop/pNXT/src

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/jimbolaptop/pNXT/src/hp-0.1/pNXTd")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/Users/jimbolaptop/pNXT/src/hp-0.1" TYPE EXECUTABLE FILES "/Users/jimbolaptop/pNXT/src/src/pNXTd")
  IF(EXISTS "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/pNXTd" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/pNXTd")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/pNXTd")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/jimbolaptop/pNXT/src/hp-0.1/simplewallet")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/Users/jimbolaptop/pNXT/src/hp-0.1" TYPE EXECUTABLE FILES "/Users/jimbolaptop/pNXT/src/src/simplewallet")
  IF(EXISTS "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/simplewallet" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/simplewallet")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/simplewallet")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/jimbolaptop/pNXT/src/hp-0.1/connectivity_tool")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/Users/jimbolaptop/pNXT/src/hp-0.1" TYPE EXECUTABLE FILES "/Users/jimbolaptop/pNXT/src/src/connectivity_tool")
  IF(EXISTS "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/connectivity_tool" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/connectivity_tool")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/Users/jimbolaptop/pNXT/src/hp-0.1/connectivity_tool")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Runtime")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_system-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_filesystem-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_thread-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_date_time-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_chrono-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_regex-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_serialization-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_atomic-mt.a;/Users/jimbolaptop/pNXT/src/hp-0.1/lib/libboost_program_options-mt.a")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/Users/jimbolaptop/pNXT/src/hp-0.1/lib" TYPE FILE FILES
    "/opt/local/lib/libboost_system-mt.a"
    "/opt/local/lib/libboost_filesystem-mt.a"
    "/opt/local/lib/libboost_thread-mt.a"
    "/opt/local/lib/libboost_date_time-mt.a"
    "/opt/local/lib/libboost_chrono-mt.a"
    "/opt/local/lib/libboost_regex-mt.a"
    "/opt/local/lib/libboost_serialization-mt.a"
    "/opt/local/lib/libboost_atomic-mt.a"
    "/opt/local/lib/libboost_program_options-mt.a"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  execute_process(COMMAND /Users/jimbolaptop/pNXT/utils/macosx_fixup.sh; ;/Users/jimbolaptop/pNXT/src/hp-0.1)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

