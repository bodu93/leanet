#include <gtest/gtest.h>
#include <leanet/atomic.h>

using namespace leanet;

TEST(ATOMIC_TEST, ATOMICINT32) {
  AtomicInt32 atom;
  EXPECT_EQ(0, atom.get());

  atom.add(1);
  EXPECT_EQ(1, atom.get());

  atom.increment();
  EXPECT_EQ(2, atom.get());

  atom.decrement();
  EXPECT_EQ(1, atom.get());

  atom.addAndGet(42);
  EXPECT_EQ(43, atom.get());

  atom.addAndGet(-42);
  EXPECT_EQ(1, atom.get());
}

TEST(ATOMIC_TEST, ATOMICINT64) {
  AtomicInt64 atom;
  EXPECT_EQ(0, atom.get());

  atom.add(1);
  EXPECT_EQ(1, atom.get());

  atom.increment();
  EXPECT_EQ(2, atom.get());

  atom.decrement();
  EXPECT_EQ(1, atom.get());

  atom.addAndGet(42);
  EXPECT_EQ(43, atom.get());

  atom.addAndGet(-42);
  EXPECT_EQ(1, atom.get());
}
