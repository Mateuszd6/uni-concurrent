#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include <stddef.h>
#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace detail {
struct Turnstile {
 private:
  std::condition_variable cv;
  size_t awaiting;

  // This is used to handle superious wakeups.
  bool can_enter;

 public:
  Turnstile();
  Turnstile(const Turnstile&) = delete;
  Turnstile(Turnstile&&) = delete;

  // NOTE: This cannot be noexcept since std::condition_variable::wait is
  // noexcept only since c++14
  void acquire(std::unique_lock<std::mutex>& lock);
  void signal() noexcept;
  size_t getAwaiting() const noexcept;
};
}  // namespace detail

struct Mutex {
 private:
  detail::Turnstile* turnstile_ptr;
  size_t getHash() const noexcept;

 public:
  Mutex();
  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;

  void lock();    // NOLINT
  void unlock();  // NOLINT
};

#endif  // SRC_TURNSTILE_H_
