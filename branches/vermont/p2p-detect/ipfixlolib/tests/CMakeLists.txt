ADD_EXECUTABLE(test_everything
	test_everything.cc
)

TARGET_LINK_LIBRARIES(test_everything
	ipfixlolib
	common
)
