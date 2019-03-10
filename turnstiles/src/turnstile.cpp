#include "turnstile.h"

#include <cassert>
#include <mutex>

namespace detail {
constexpr static int g_no_guards = 256;
static std::mutex g_guards[g_no_guards] = {};

// When mutex is owned but noone is waiting we dont want to create a
// Turnstile object so we mark is as owned by asiging the member poitner to
// the addres of this variable. This is not really necesarry, I could have
// used the low bits of a poitner to incidate that, which wouldn't waste the
// stack space, but this would make the solution "less portable" so I
// decided not to.
static Turnstile g_dummy_turnstile;

Turnstile::Turnstile() {
  awaiting = 0;
  can_enter = false;
}

void Turnstile::acquire(std::unique_lock<std::mutex>& lock) {
  this->awaiting++;
  this->cv.wait(lock, [this]() noexcept { return can_enter == true; });

  this->awaiting--;
  this->can_enter = false;
}

void Turnstile::signal() noexcept {
  this->can_enter = true;
  this->cv.notify_one();
}

size_t Turnstile::getAwaiting() const noexcept { return awaiting; }
}  // namespace detail

size_t Mutex::getHash() const noexcept {
  auto hash_val = std::hash<size_t>{}(reinterpret_cast<size_t>(this));
  return hash_val % 256;
}

Mutex::Mutex() { this->turnstile_ptr = nullptr; }

void Mutex::lock() {
  auto this_hash = getHash();
  std::unique_lock<std::mutex> lock{detail::g_guards[this_hash]};

  if (this->turnstile_ptr == nullptr) {
    this->turnstile_ptr = &detail::g_dummy_turnstile;
  } else if (this->turnstile_ptr == &detail::g_dummy_turnstile) {
    this->turnstile_ptr = new detail::Turnstile();
    this->turnstile_ptr->acquire(lock);

    if (this->turnstile_ptr->getAwaiting() == 0) {
      delete this->turnstile_ptr;
      this->turnstile_ptr = &detail::g_dummy_turnstile;
    }
  } else {
    this->turnstile_ptr->acquire(lock);
    if (this->turnstile_ptr->getAwaiting() == 0) {
      delete this->turnstile_ptr;
      this->turnstile_ptr = &detail::g_dummy_turnstile;
    }
  }
}

void Mutex::unlock() {
  auto this_hash = getHash();
  std::unique_lock<std::mutex> lock{detail::g_guards[this_hash]};

  if (this->turnstile_ptr == nullptr) {
    assert(!"Invalid use of mutex!");
  } else if (this->turnstile_ptr == &detail::g_dummy_turnstile) {
    this->turnstile_ptr = nullptr;
  } else {
    assert(this->turnstile_ptr->getAwaiting() > 0);
    this->turnstile_ptr->signal();
  }
}
