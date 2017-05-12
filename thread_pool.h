#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>

class thread_task {
 public:
  virtual ~thread_task() {}
  virtual bool run() = 0;
};

class thread_pool {
 public:
  explicit thread_pool(const unsigned& thread_num)
      : thread_num_(thread_num), threads_(NULL) {
    pthread_mutex_init(&queue_mutex_, NULL);
    sem_init(&task_sem_, 0, 0);
    sem_init(&quit_sem_, 0, 0);
    threads_ = new pthread_t[thread_num_];
  }

  virtual ~thread_pool() {
    sem_destroy(&quit_sem_);
    sem_destroy(&task_sem_);
    pthread_mutex_destroy(&queue_mutex_);
    if (threads_) {
      delete[] threads_;
      threads_ = NULL;
    }
  }

  inline bool start() {
    for (unsigned i = 0; i < thread_num_; ++i) {
      int ret = pthread_create(threads_ + i, NULL, thread_proc, this);
      if (ret) {
        return false;
      }
    }

    return true;
  }

  inline void join() {
    for (unsigned i = 0; i < thread_num_; ++i) {
      pthread_join(threads_[i], NULL);
    }
  }

  inline void detach() {
    for (unsigned i = 0; i < thread_num_; ++i) {
      pthread_detach(threads_[i]);
    }
  }

  inline void stop() {
    for (unsigned i = 0; i < thread_num_; ++i) {
      sem_post(&quit_sem_);
    }
  }

  inline void add_task(thread_task* task) {
    pthread_mutex_lock(&queue_mutex_);
    tasks_.push(task);
    pthread_mutex_unlock(&queue_mutex_);
    sem_post(&task_sem_);
  }

  inline static void* thread_proc(void* ptr) {
    thread_pool* pthis = reinterpret_cast<thread_pool*>(ptr);
    struct timespec ts;
    while (true) {
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_nsec += 1000 * 1000 * 10;
      int quit_wait = sem_timedwait(&pthis->quit_sem_, &ts);
      if (0 == quit_wait) {
        break;
      } else if (-1 == quit_wait && ETIMEDOUT == errno) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1000 * 1000 * 10;
        if (0 == sem_timedwait(&pthis->task_sem_, &ts)) {
          thread_task* task = NULL;
          pthread_mutex_lock(&pthis->queue_mutex_);
          if (pthis->tasks_.size() > 0) {
            task = pthis->tasks_.front();
            pthis->tasks_.pop();
          }
          pthread_mutex_unlock(&pthis->queue_mutex_);

          if (task) {
            task->run();
          }
        }
      }
    }

    return NULL;
  }

 protected:
  unsigned thread_num_;
  pthread_t* threads_;
  pthread_mutex_t queue_mutex_;
  std::queue<thread_task*> tasks_;
  sem_t task_sem_;
  sem_t quit_sem_;
};

#endif
