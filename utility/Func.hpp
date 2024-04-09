#ifndef MYCOROUTINE_FUNC_HPP
#define MYCOROUTINE_FUNC_HPP
#include <iostream>
#include <functional>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h> // Include the header file for pthread_getthreadid_np function

namespace myCoroutine {
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}
}
#endif // MYCOROUTINE_FUNC_HPP