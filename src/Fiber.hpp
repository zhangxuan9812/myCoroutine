#ifndef MYCOROUTINE_FIBER_HPP
#define MYCOROUTINE_FIBER_HPP
//#include "Scheduler.hpp"
#include <memory>
#define _XOPEN_SOURCE // Add this line to define _XOPEN_SOURCE
#include <ucontext.h>
#include <iostream>
#include <functional> // Include the <functional> header
#include <atomic> // Include the <atomic> header
namespace myCoroutine {
class Scheduler;
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};
static const size_t default_stacksize = 128 * 1024; // The default size of the stack is 128KB


class Fiber : std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum class State { // Define the state of the coroutine
        READY, // The coroutine is ready to run
        RUNNING, // The coroutine is running
        DEAD // The coroutine is dead
    };
private:
    Fiber();
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true); 
    ~Fiber();
    void reset(std::function<void()> cb);
    void resume();
    void yield();
    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }
    static void SetThis(Fiber *f);
    static Fiber::ptr GetThis();
    static uint64_t TotalFibers();
    static void MainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0; // The id of the coroutine
    uint32_t m_stacksize = 0; // The size of the stack
    State m_state = State::READY; // The state of the coroutine
    ucontext_t m_ctx; // The context of the coroutine
    void *m_stack = nullptr; // The stack of the coroutine
    std::function<void()> m_cb; // The callback function of the coroutine
    bool m_runInScheduler; // Whether the coroutine runs in the scheduler
};
// Define the thread_local variables
static thread_local Fiber *t_fiber = nullptr; // The coroutine of the current thread
static thread_local Fiber::ptr t_thread_fiber = nullptr; // The main coroutine of the current thread
} // namespace myCoroutine

#endif