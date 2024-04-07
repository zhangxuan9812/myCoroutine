#ifndef FIBER_HPP
#define FIBER_HPP

#include <memory>
#define _XOPEN_SOURCE // Add this line to define _XOPEN_SOURCE
#include <ucontext.h>
#include <iostream>
#include <functional> // Include the <functional> header
namespace myCoroutine {
class Fiber;
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


// The default constructor of the coroutine, which initializes the context of the coroutine by calling SetThis(this)
Fiber::Fiber() {
    SetThis(this); // Set the current coroutine to this
    m_state = State::RUNNING;
    // When getcontext() returns 0, it returns the current context
    // When getcontext() returns -1, it indicates an error
    if (getcontext(&m_ctx)) { 
        std::cout << "getcontext error" << std::endl;   
    }
    ++s_fiber_count; // Increase the number of coroutines
    m_id = s_fiber_id++; // Assign the id to the coroutine
    std::cout << "Fiber::Fiber id=" << m_id << std::endl;
}


Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++)
    , m_cb(cb)
    , m_runInScheduler(run_in_scheduler) {
    ++s_fiber_count; // Increase the number of coroutines
    m_stacksize = stacksize ? stacksize : default_stacksize; // Set the size of the stack
    m_stack = malloc(m_stacksize); // Allocate the stack of the coroutine

    if (getcontext(&m_ctx)) {
        std::cout << "getcontext error" << std::endl;
    }
    // Set the context of the coroutine
    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    // Set the callback function of the coroutine
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    std::cout << "Fiber::Fiber id=" << m_id << std::endl; // Print the id of the coroutine
}


Fiber::ptr Fiber::GetThis() {
    if (t_fiber) { // If the current coroutine is not null
        return t_fiber->shared_from_this(); // Return the shared pointer of the current coroutine
    }
    Fiber::ptr main_fiber(new Fiber()); //Create the main coroutine
    t_thread_fiber = main_fiber; // Set the main coroutine to the current thread
    return t_fiber->shared_from_this(); // Return the shared pointer of the main coroutine
}


void Fiber::SetThis(Fiber* f) { // Set the current coroutine to f
    t_fiber = f;
}


// Destructor of the coroutine
Fiber::~Fiber() {
    std::cout << "Fiber::~Fiber id=" << m_id << std::endl;
    --s_fiber_count;
    if (m_stack) {
        if (this->m_state != State::DEAD) { // If the coroutine is not dead
            throw std::runtime_error("Fiber is not dead");
        }
        free(m_stack); // Free the stack of the coroutine
    } else {
        if (m_cb) {
            throw std::runtime_error("Fiber is not main fiber!");
        }
        if (!(m_state == State::RUNNING)) {
            throw std::runtime_error("Fiber is not running!");
        }
        Fiber *cur = t_fiber; // Get the current coroutine
        if (cur == this) { // If the current coroutine is this
            SetThis(nullptr); // Set the current coroutine to null
        }
    }
}


void Fiber::reset(std::function<void()> cb) {
    if (!(m_stack || m_state == State::DEAD)) {
        throw std::runtime_error("Fiber is not dead!");
    }
    m_cb = cb; // Set the callback function of the coroutine
    if (getcontext(&m_ctx)) {
        throw std::runtime_error("getcontext error");
    }
    // Set the context of the coroutine
    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    // Set the callback function of the coroutine
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = State::READY; // Set the state of the coroutine to ready
}

void Fiber::resume() {
    if (m_state == State::RUNNING || m_state == State::DEAD) {
        throw std::runtime_error("resume error");
    }
    SetThis(this);
    m_state = State::RUNNING;
    if (m_runInScheduler) {
        if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
           std::cout << "swapcontext error" << std::endl;
        }
    } else {
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
            std::cout << "swapcontext error" << std::endl;
        }
    }
}

void Fiber::yield() {
    if (m_state == State::READY) {
        throw std::runtime_error("yield error");
    }
    SetThis(t_thread_fiber.get());
    if (m_state != State::DEAD) {
        m_state = State::READY;
    }
    if (m_runInScheduler) {
        if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
            std::cout << "swapcontext error" << std::endl;
        }
    } else {
        if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx))) {
            std::cout << "swapcontext error" << std::endl;
        }
    }
}


void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis(); // GetThis()的shared_from_this()方法让引用计数加1
    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = State::DEAD;

    auto raw_ptr = cur.get(); 
    cur.reset();
    raw_ptr->yield();
}
} // namespace myCoroutine

#endif