#include <leanet/blockingqueue.h>
#include <leanet/boundedblockingqueue.h>
#include <leanet/thread.h>
#include <leanet/currentthread.h>
#include <leanet/countdownlatch.h>
#include <leanet/timestamp.h>
#include <gtest/gtest.h>

#include <unistd.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <string>

template<
  template<typename T>
  class QUEUE
>
class BlockingQueueTest {
public:
  explicit BlockingQueueTest(int threadsCount)
    : latch_(threadsCount)
  {
    for (int i = 0; i < threadsCount; ++i) {
      char name[32];
      snprintf(name, sizeof(name), "work thread %d", i);
      std::unique_ptr<leanet::Thread> thd(new leanet::Thread(std::bind(&BlockingQueueTest::threadFunc, this), std::string(name)));
      threads_.push_back(std::move(thd));
    }

    std::for_each(threads_.begin(), threads_.end(),
        std::bind(&leanet::Thread::start, std::placeholders::_1));
  }

  void run(int times) {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started!\n");
    for (int i = 0; i < times; ++i) {
      char buf[32];
      snprintf(buf, sizeof(buf), "hello %d", i);
      queue_.put(buf);
      printf("tid=%lu, put data = %s, size = %zd\n",
          leanet::currentThread::tid(), buf, queue_.size());
    }
  }

  void joinAll() {
    for (size_t i = 0; i < threads_.size(); ++i) {
      queue_.put("stop");
    }

    std::for_each(threads_.begin(), threads_.end(),
        std::bind(&leanet::Thread::join, std::placeholders::_1));
  }

private:
  void threadFunc() {
    printf("tid=%lu, %s started\n",
        leanet::currentThread::tid(),
        leanet::currentThread::name());

    latch_.countDown();
    bool running = true;
    while (running) {
      std::string data(queue_.get());
      printf("tid=%lu, get data = %s, size = %zd\n",
          leanet::currentThread::tid(), data.c_str(), queue_.size());
      running = (data != "stop");
    }

    printf("tid=%lu %s stopped\n",
        leanet::currentThread::tid(),
        leanet::currentThread::name());
  }

  QUEUE<std::string> queue_;
  leanet::CountdownLatch latch_;
  std::vector<std::unique_ptr<leanet::Thread>> threads_;
};

class BlockingQueueBench {
public:
  explicit BlockingQueueBench(int threadsCount)
    : latch_(threadsCount)
  {
    for (int i = 0; i < threadsCount; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "work thread %d", i);
      std::unique_ptr<leanet::Thread> thd(new leanet::Thread(std::bind(&BlockingQueueBench::threadFunc, this), buf));
      threads_.push_back(std::move(thd));
    }
    std::for_each(threads_.begin(), threads_.end(), std::bind(&leanet::Thread::start, std::placeholders::_1));
  }

  void run(int times) {
    printf("waiting for countdownlatch\n");
    latch_.wait();
    printf("all threads started\n");

    for (int i = 0; i < times; ++i) {
      queue_.put(leanet::Timestamp::now());
      usleep(1000);
    }
  }

  void joinAll() {
    for (size_t i = 0; i < threads_.size(); ++i) {
      queue_.put(leanet::Timestamp::invalid());
    }
    // detach all threads
    std::for_each(threads_.begin(), threads_.end(), std::bind(&leanet::Thread::join, std::placeholders::_1));
  }

private:
  void threadFunc() {
    printf("thread %s, tid %lu started\n", leanet::currentThread::name(), leanet::currentThread::tid());
    latch_.countDown();

    std::map<int, int> delays;
    bool running = true;
    while (running) {
      leanet::Timestamp t(queue_.get());
      running = t.valid();
      if (running) {
        leanet::Timestamp now(leanet::Timestamp::now());
        // delay seconds
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        ++delays[delay];
      }
    }

    printf("thread %s, tid %lu stopped\n", leanet::currentThread::name(), leanet::currentThread::tid());
    for (std::map<int, int>::iterator iter = delays.begin();
         iter != delays.end();
         ++iter) {
      printf("thread %s, tid %lu, delay %d seconds, count %d\n",
          leanet::currentThread::name(),
          leanet::currentThread::tid(),
          iter->first,
          iter->second);
    }
  }

  leanet::BlockingQueue<leanet::Timestamp> queue_;
  leanet::CountdownLatch latch_;
  std::vector<std::unique_ptr<leanet::Thread>> threads_;
};

int main() {
  {
  // test BlockingQueue class
  printf("pid=%d, tid=%lu\n", ::getpid(), leanet::currentThread::tid());
  BlockingQueueTest<leanet::BlockingQueue> test(10);
  test.run(100);
  test.joinAll();

  printf("number of created threads %d\n", leanet::Thread::threadsCreated());
  }

  {
  // test BoundedBlockingQueue class
  printf("pid=%d, tid=%lu\n", ::getpid(), leanet::currentThread::tid());
  BlockingQueueTest<leanet::BoundedBlockingQueue> test(10);
  test.run(100);
  test.joinAll();

  printf("number of created threads %d\n", leanet::Thread::threadsCreated());
  }

  {
  // bench BlockingQueue classs
  printf("pid = %d, tid=%lu\n", ::getpid(), leanet::currentThread::tid());
  BlockingQueueBench bench(10);
  bench.run(100);
  bench.joinAll();
  printf("number of created threads %d\n", leanet::Thread::threadsCreated());
  }

  return 0;
}
