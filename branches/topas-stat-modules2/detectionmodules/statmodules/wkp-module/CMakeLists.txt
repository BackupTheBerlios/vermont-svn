ADD_EXECUTABLE(wkp-module main.cpp stat-main.cpp stat-store.cpp wmw-test.cpp ks-test.cpp pcs-test.cpp cusum-test.cpp shared.cpp params.cpp)
INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(wkp-module detectionBase commonUtils ipfixCollector ${LIBXML2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GSL_LIBRARIES})
