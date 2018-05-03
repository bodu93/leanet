#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <leanet/thread.h>
#include <leanet/currentthread.h>
#include <leanet/eventloop.h>

using namespace leanet;

void threadFunc() {
	printf("threadFunc(): pid = %d, tid = %lu\n", getpid(), leanet::currentThread::tid());
	leanet::EventLoop loop;
	loop.loop();
}

int main() {
	printf("main(): pid = %d, tid = %lu\n", getpid(), leanet::currentThread::tid());

	leanet::EventLoop loop;

	leanet::Thread thread(threadFunc);
	thread.start();

	loop.loop();
	pthread_exit(NULL);
}
