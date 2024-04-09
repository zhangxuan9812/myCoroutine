#ifndef MYCOROUTINE_SCHEDULER_HPP
#define MYCOROUTINE_SCHEDULER_HPP
#include "Fiber.hpp"
#include "Noncopyable.hpp"
#include "Thread.hpp"
#include <memory>
#include <mutex>
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
namespace myCoroutine {
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef std::mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "Scheduler");
    virtual ~Scheduler();
    const std::string &getName() const { return m_name; }
    static Scheduler *GetThis();
    static Fiber *GetMainFiber();
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            std::lock_guard<MutexType> lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if (need_tickle) {
            tickle(); 
        }
    }
    void start();
    void stop();
protected:
    virtual void tickle();
    void run();
    virtual void idle();
    virtual bool stopping();
    void setThis();
    bool hasIdleThreads() { return m_idleThreadCount > 0; }
private:

    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_tasks.empty();
        ScheduleTask task(fc, thread);
        if (task.fiber || task.cb) {
            m_tasks.push_back(task);
        }
        return need_tickle;
    }
private:
    struct ScheduleTask {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        ScheduleTask(Fiber::ptr f, int thr) {
            fiber  = f;
            thread = thr;
        }
        ScheduleTask(Fiber::ptr *f, int thr) {
            fiber.swap(*f);
            thread = thr;
        }
        ScheduleTask(std::function<void()> f, int thr) {
            cb     = f;
            thread = thr;
        }
        ScheduleTask() { thread = -1; }

        void reset() {
            fiber  = nullptr;
            cb     = nullptr;
            thread = -1;
        }
    };
private:

    std::string m_name;

    MutexType m_mutex;

    std::vector<Thread::ptr> m_threads;

    std::list<ScheduleTask> m_tasks;

    std::vector<int> m_threadIds;

    size_t m_threadCount = 0;

    std::atomic<size_t> m_activeThreadCount = {0};

    std::atomic<size_t> m_idleThreadCount = {0};

    bool m_useCaller;

    Fiber::ptr m_rootFiber;

    int m_rootThread = 0;

    bool m_stopping = false;
};
}
#endif