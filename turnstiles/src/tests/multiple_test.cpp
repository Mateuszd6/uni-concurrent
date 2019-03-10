#include <cassert>
#include <thread>
#include <vector>
#include "turnstile.h"

const size_t THREADS_PER_PATH = 20;
const int REPEATS = 10000;

Mutex m1, m2, m3, m4, m5, m6, m7, m8;

int result = 0;

void path1() {
  for (int i = 0; i < REPEATS; ++i) {
    m1.lock();
    m2.lock();
    m4.lock();
    m5.lock();
    m6.lock();
    m8.lock();

    ++result;

    m2.unlock();
    m4.unlock();
    m1.unlock();
    m5.unlock();
    m6.unlock();
    m8.unlock();
  }
}

void path2() {
  for (int i = 0; i < REPEATS; ++i) {
    m3.lock();
    m2.lock();
    m4.lock();
    m6.lock();
    m5.lock();
    m7.lock();
    m8.lock();

    ++result;

    m3.unlock();
    m2.unlock();
    m4.unlock();
    m6.unlock();
    m5.unlock();
    m7.unlock();
    m8.unlock();
  }
}

int main() {
  std::vector<std::thread> threads;
  threads.reserve(THREADS_PER_PATH * 2);

  for (size_t i = 0; i < THREADS_PER_PATH; ++i) {
    threads.emplace_back(path1);
    threads.emplace_back(path2);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  assert(result == 2 * THREADS_PER_PATH * REPEATS);
}
