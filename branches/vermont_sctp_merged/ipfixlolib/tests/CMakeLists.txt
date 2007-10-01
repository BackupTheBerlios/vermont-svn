ADD_EXECUTABLE(test_everything
	test_everything.c
)

TARGET_LINK_LIBRARIES(test_everything
	ipfixlolib
	common
)
