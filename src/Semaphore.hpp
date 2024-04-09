#ifndef MYCOROUTINE_SEMAPHORE_HPP
#define MYCOROUTINE_SEMAPHORE_HPP
#include "Noncopyable.hpp"
//#include "Fiber.hpp"
#include <mutex>
#include <iostream>
#include <list>
#include <assert.h>
#include <semaphore.h>
namespace myCoroutine {
// The Semaphore class
class Semaphore : Noncopyable {
public:
    Semaphore(uint32_t count = 0); // Set the initial value of the semaphore
    ~Semaphore();
    void wait();
    void notify();
private:
    sem_t m_semaphore; // The semaphore
};
Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}
}
#endif // MYCOROUTINE_SEMAPHORE_HPP