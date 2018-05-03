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
	threadpool.cc
	logstream.cc
	logger.cc

	buffer.cc
	sockets.cc
	socket.cc
	inetaddress.cc
	timer.cc
	# timerqueue.cc
	channel.cc
	poller.cc
	# eventloop.cc
	eventloopthread.cc
	eventloopthreadpool.cc
	acceptor.cc
	tcpconnection.cc
	tcpserver.cc
	connector.cc
	tcpclient.cc
	)

add_library(leanet ${SRCS})
#target_link_libraries(leanet pthread rt)
target_link_libraries(leanet pthread)
