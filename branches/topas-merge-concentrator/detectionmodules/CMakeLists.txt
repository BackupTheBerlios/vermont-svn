SUBDIRS(detectionbase examplemodules snortmodule statmodules countmodule)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/detectionmodules/detectionbase/)

IF (XML_BLASTER_FOUND)
  INCLUDE_DIRECTORIES(${XML_BLASTER_INCLUDE_DIR})
  LINK_LIBRARIES(${XML_BLASTER_C_LIBRARIES} ${XML_BLASTER_CPP_LIBRARIES} ${XERCES_LIBRARIES})
ENDIF (XML_BLASTER_FOUND)
