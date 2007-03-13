ADD_EXECUTABLE(sw-module main.cpp sw-base.cpp sw-store.cpp stuff.cpp)
TARGET_LINK_LIBRARIES(sw-module detectionBase commonUtils ipfixCollector ${LIBXML2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
