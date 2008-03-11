ADD_EXECUTABLE(print-module main.cpp print-main.cpp print-store.cpp)
TARGET_LINK_LIBRARIES(print-module detectionBase commonUtils concentrator ${LIBXML2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
