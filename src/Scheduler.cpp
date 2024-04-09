#include "Scheduler.hpp"
#include "Func.hpp"
#include "Fiber.hpp"
#include "Semaphore.hpp"
#include "Thread.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
namespace myCoroutine {
static thread_local Scheduler *t_scheduler = nullptr; // Current scheduler
static thread_local Fiber *t_scheduler_fiber = nullptr; // The main coroutine of the scheduler

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {
    assert(threads > 0);
    m_useCaller = use_caller;
    m_name      = name;

    if (use_caller) {
        --threads;
        myCoroutine::Fiber::GetThis();
        assert(GetThis() == nullptr);
        t_scheduler = this;
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

        myCoroutine::Thread::SetName(m_name);
        t_scheduler_fiber = m_rootFiber.get();
        //m_rootThread      = std::this_thread::get_id(); // Fix: Change data type to '__thread_id'
        m_rootThread = myCoroutine::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1; 
    }
    m_threadCount = threads;
}

Scheduler *Scheduler::GetThis() { 
    return t_scheduler; 
}

Fiber *Scheduler::GetMainFiber() { 
    return t_scheduler_fiber;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

Scheduler::~Scheduler() {
    std::cout << "Scheduler::~Scheduler()" << std::endl;
    assert(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

void Scheduler::start() {
    std::cout << "Scheduler::start()" << std::endl;
    std::lock_guard<MutexType> lock(m_mutex);
    if (m_stopping) {
        std::cout << "Scheduler::start() m_stopping" << std::endl;
        return;
    }
    assert(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; i++) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                      m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

bool Scheduler::stopping() {
    std::lock_guard<MutexType> lock(m_mutex);
    return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::tickle() { 
    std::cout << "tickle" << std::endl;
}

void Scheduler::idle() {
    std::cout << "idle" << std::endl;
    while (!stopping()) {
        myCoroutine::Fiber::GetThis()->yield();
    }
}

void Scheduler::stop() {
    std::cout << "stop" << std::endl;
    if (stopping()) {
        return;
    }
    m_stopping = true;

    /// 如果use caller，那只能由caller线程发起stop
    if (m_useCaller) {
        assert(GetThis() == this);
    } 
    else {
        assert(GetThis() != this);
    }

    for (size_t i = 0; i < m_threadCount; i++) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        m_rootFiber->resume();
        std::cout << "m_rootFiber end" << std::endl;
    }

    std::vector<Thread::ptr> thrs;
    {
        std::lock_guard<MutexType> lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto &i : thrs) {
        i->join();
    }
}

void Scheduler::run() {
    std::cout << "run" << std::endl;
    //set_hook_enable(true);
    setThis();
    if (myCoroutine::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = myCoroutine::Fiber::GetThis().get();
    }
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    ScheduleTask task;
    while (true) {
        task.reset();
        bool tickle_me = false; 
        {
            std::lock_guard<MutexType> lock(m_mutex);
            auto it = m_tasks.begin();

            while (it != m_tasks.end()) {
                if (it->thread != -1 && it->thread != myCoroutine::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }
                assert(it->fiber || it->cb); // The fiber or callback function must exist
                if(it->fiber && it->fiber->getState() == Fiber::State::RUNNING) {
                    ++it;
                    continue;
                }
                task = *it;
                m_tasks.erase(it++);
                ++m_activeThreadCount;
                break;
            }
            tickle_me |= (it != m_tasks.end());
        }
        if (tickle_me) {
            tickle();
        }
        if (task.fiber) {
            task.fiber->resume();
            --m_activeThreadCount;
            task.reset();
        } else if (task.cb) {
            if (cb_fiber) {
                cb_fiber->reset(task.cb);
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reset();
            cb_fiber->resume();
            --m_activeThreadCount;
            cb_fiber.reset();
        } else {
            if (idle_fiber->getState() == Fiber::State::DEAD) {
                std::cout << "idle fiber dead" << std::endl;
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->resume();
            --m_idleThreadCount;
        }
    }
    std::cout << "Scheduler::run() end" << std::endl;
}
}