PROJECT(COLLECTOR)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SUBDIRS(concentrator commonutils collector detectionmodules)

FIND_PACKAGE(Threads)
FIND_PACKAGE(Doxygen)

#################################### what about doxygen? ######################################

IF (DOXYGEN)
  MESSAGE(STATUS "Found doxygen")
ELSE (DOXYGEN)
  MESSAGE(STATUS "Could not find doxygen")
ENDIF (DOXYGEN)


#################################### check for pthreads ########################################

IF (CMAKE_USE_PTHREADS_INIT)
  MESSAGE(STATUS "Found pthread library")
ELSE (CMAKE_USE_PTHREADS_INIT)
  MESSAGE(FATAL_ERROR "Could not find a suitable pthread library")
ENDIF (CMAKE_USE_PTHREADS_INIT)


#################################### Locate libxml2 ############################################

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/modules/FindLibXML2.cmake)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR})
LINK_DIRECTORIES(${CMAKE_BUILD_DIR}/concentrator ${CMAKE_BUILD_DIR}/commonutils)

#################################### Locate gsl ################################################

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/modules/FindGSL.cmake)

#################################### boost ####################################################

FIND_PACKAGE(Boost REQUIRED)
MARK_AS_ADVANCED(
        Boost_INCLUDE_DIR
        Boost_REGEX_LIBRARY
        Boost_FILESYSTEM_LIBRARY
        Boost_UNIT_TEST_FRAMEWORK_LIBRARY
)
IF (Boost_FOUND)
        MESSAGE(STATUS "Found boost libraries")
        ADD_DEFINITIONS(-DHAVE_BOOST_FILESYSTEM)
        INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
        FIND_LIBRARY(Boost_REGEX_LIBRARY NAMES boost_regex-mt boost_regex PATHS ${Boost_LIBRARY_DIRS})
        FIND_LIBRARY(Boost_FILESYSTEM_LIBRARY NAMES boost_filesystem-mt boost_filesystem PATHS ${Boost_LIBRARY_DIRS})
        FIND_LIBRARY(Boost_UNIT_TEST_FRAMEWORK_LIBRARY NAMES boost_unit_test_framework-mt boost_unit_test_framework PATHS ${Boost_LIBRARY_DIRS})
        TARGET_LINK_LIBRARIES(vermont
                ${Boost_REGEX_LIBRARY}
                ${Boost_FILESYSTEM_LIBRARY}
        )
ELSE (Boost_FOUND)
        MESSAGE(FATAL_ERROR "Could not find boost libraries")
        REMOVE_DEFINITIONS(-DHAVE_BOOST_FILESYSTEM)
ENDIF (Boost_FOUND)


#################################### Look for xmlBlaster #######################################

OPTION(IDMEF "Enable/Disable IDMEF-Support. Requires xmlBlaster if enabled." ON)

IF (IDMEF)
  #### Check for libXerces ####
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/modules/FindXerces.cmake)
  message(STATUS "Looking for xmlBLaster...")
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/modules/FindXmlBlaster.cmake)
  IF (NOT XERCES_FOUND)
    message(FATAL_ERROR "Xerces not installed! ${XERCES_FOUND}")
  ENDIF (NOT XERCES_FOUND)
  message(STATUS "Applying defines...")
  ADD_DEFINITIONS(-DIDMEF_SUPPORT_ENABLED)
ELSE (IDMEF)
  REMOVE_DEFINITIONS(-DIDMEF_SUPPORT_ENABLED)
ENDIF (IDMEF)

ADD_DEFINITIONS(-g -pg)

################################### Stuff for concentrator ####################################


### IP_HEADER_OFFSET

SET(IP_HEADER_OFFSET 14 CACHE STRING "Start position of the IP header in an ethernet frame in Bytes. This value needs to be adjusted according to the network monitored. The default value is 14 for ethernet devices. Other common values are 4 (BSD loop back device) and 18 (Ethernet VLAN)")
ADD_DEFINITIONS(-DIP_HEADER_OFFSET=${IP_HEADER_OFFSET})


### PCAP_MAX_CAPTURE_LENGTH

SET(PCAP_MAX_CAPTURE_LENGTH 128 CACHE STRING "Maximum PCAP packet capture length (this amount of bytes is always allocated for each packet, the smaller the better!)")
ADD_DEFINITIONS(-DPCAP_MAX_CAPTURE_LENGTH=${PCAP_MAX_CAPTURE_LENGTH})


