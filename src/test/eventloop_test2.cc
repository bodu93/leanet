#include "../eventloop.h"
#include "../thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

leanet::EventLoop* g_loop;

void threadFunc() {
	g_loop->loop();
}

int main() {
	leanet::EventLoop loop;
	g_loop = &loop;
	leanet::Thread t(threadFunc);
	t.start();
	t.join();

	return 0;
}
