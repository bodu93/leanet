include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if (NOT HAVE_ACCEPT4)
	set_source_files_properties(sockets.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(SRCS
	timestamp.cc
	timezone.cc
	date.cc
	thread.cc
	logstream.cc
	logger.cc

	buffer.cc
	sockets.cc
	socket.cc
	inetaddress.cc
	channel.cc
	eventloop.cc
	)

add_library(leanet ${SRCS})
#target_link_libraries(leanet pthread rt)
target_link_libraries(leanet pthread)
