#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
#include "turnstile.h"

template <typename T, typename M>
class Buffer {
 private:
  static const size_t SIZE{20};

  std::array<T, SIZE> buffer{};
  size_t begin{0};
  size_t end{0};

  M mutex{};
  std::condition_variable_any producers{};
  std::condition_variable_any consumers{};

 public:
  Buffer() {}
  Buffer(const Buffer bitand) = delete;

  void insert(T t) {
    std::unique_lock<M> lock{mutex};

    producers.wait(lock, [this]() { return begin != (end + 1) % SIZE; });

    buffer[end] = t;
    end = (end + 1) % SIZE;

    consumers.notify_all();
  }

  T extract() {
    std::unique_lock<M> lock{mutex};

    consumers.wait(lock, [this]() { return begin != end; });

    T t = buffer[begin];
    begin = (begin + 1) % SIZE;

    producers.notify_all();

    return t;
  }
};

template <typename T, typename M>
void produceToBuffer(Buffer<T, M>& buffer, size_t count) {
  for (size_t i = 1; i <= count; ++i) {
    buffer.insert(i);
  }
}

template <typename T, typename M>
void sumFromBuffer(Buffer<T, M>& buffer, size_t count,
                   std::atomic<T>& cumulativeSum) {
  T sum = 0;

  for (size_t i = 1; i <= count; ++i) {
    sum += buffer.extract();
  }
  cumulativeSum += sum;
}

template <typename T, typename M>
void test(size_t producers, size_t consumers, size_t count) {
  std::cout << "Beginning test with " << producers << " producers, "
            << consumers << " consumers and a total of " << count * producers
            << " portions" << std::endl;

  Buffer<T, M> buffer{};
  std::vector<std::thread> threads;
  threads.reserve(producers + consumers);

  assert((producers * count) % consumers == 0);
  auto consumerLoad = producers * count / consumers;
  std::atomic<T> sum{0};

  for (size_t i = 0; i < producers; ++i) {
    threads.emplace_back(
        [&buffer, count] { produceToBuffer<T>(buffer, count); });
  }
  for (size_t i = 0; i < consumers; ++i) {
    threads.emplace_back([&buffer, consumerLoad, &sum] {
      sumFromBuffer<T>(buffer, consumerLoad, sum);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  assert(sum == ((count + 1) * count) / 2 * producers);

  std::cout << "Test passed." << std::endl;
}

int main() {
  test<unsigned, Mutex>(1, 1, 10000);
  test<unsigned, Mutex>(1, 10, 10000);
  test<unsigned, Mutex>(10, 1, 10000);
  test<unsigned, Mutex>(10, 10, 10000);
  test<unsigned, Mutex>(20, 10, 10000);
  test<unsigned, Mutex>(10, 20, 10000);

  std::cout << "Performance comparison..." << std::endl;

  auto startMutex = std::chrono::high_resolution_clock::now();
  test<unsigned long long, Mutex>(20, 30, 9000);
  auto stopMutex = std::chrono::high_resolution_clock::now();
  auto startStd = std::chrono::high_resolution_clock::now();
  test<unsigned long long, std::mutex>(20, 30, 9000);
  auto stopStd = std::chrono::high_resolution_clock::now();

  std::cout << "std::mutex time: " << (stopStd - startStd).count() * 0.000000001
            << " s" << std::endl;
  std::cout << "Mutex time:      "
            << (stopMutex - startMutex).count() * 0.000000001 << " s"
            << std::endl;

  return 0;
}
