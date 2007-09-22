# Install script for directory: /home/alex/uni/vermont_merged

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/alex/uni/vermont_merged/CMakeFiles/CMakeRelink.dir/vermont")
FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/vermont" TYPE FILE FILES "/home/alex/uni/vermont_merged/README")
FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/vermont" TYPE FILE FILES "/home/alex/uni/vermont_merged/CONFIGURATION")
FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/vermont" TYPE FILE FILES "/home/alex/uni/vermont_merged/LICENSE")
FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/vermont" TYPE FILE FILES "/home/alex/uni/vermont_merged/ipfix-config-schema.xsd")
IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/home/alex/uni/vermont_merged/common/cmake_install.cmake")
  INCLUDE("/home/alex/uni/vermont_merged/concentrator/cmake_install.cmake")
  INCLUDE("/home/alex/uni/vermont_merged/ipfixlolib/cmake_install.cmake")
  INCLUDE("/home/alex/uni/vermont_merged/sampler/cmake_install.cmake")
  INCLUDE("/home/alex/uni/vermont_merged/tools/cmake_install.cmake")
  INCLUDE("/home/alex/uni/vermont_merged/tests/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)
IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)
FILE(WRITE "/home/alex/uni/vermont_merged/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/home/alex/uni/vermont_merged/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
