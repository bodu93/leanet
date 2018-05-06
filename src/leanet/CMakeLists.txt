include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if (NOT HAVE_ACCEPT4)
	set_source_files_properties(sockets.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(SRCS
	acceptor.cc
	buffer.cc
	channel.cc
	# circularbuffer.cc
	connector.cc
	date.cc
	epollpoller.cc
	eventloop.cc
	eventloopthread.cc
	eventloopthreadpool.cc
	inetaddress.cc
	logger.cc
	logstream.cc
	poller.cc
	# posix.cc
	socket.cc
	sockets.cc
	tcpclient.cc
	tcpconnection.cc
	tcpserver.cc
	thread.cc
	threadpool.cc
	timer.cc
	# timerfd.cc
	timerqueue.cc
	timestamp.cc
	timezone.cc
	)

add_library(leanet ${SRCS})
target_link_libraries(leanet pthread rt)
