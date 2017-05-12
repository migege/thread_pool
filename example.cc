#include <iostream>
#include "thread_pool.h"

class print_task : public thread_task {
 public:
  explicit print_task(int task_id) : task_id_(task_id) {}
  bool run() {
    std::cout << task_id_ << std::endl;
    return true;
  }

 private:
  int task_id_;
};

class memory_task : public thread_task {
 public:
  bool run() {
    sleep(3);
    void* mem = malloc(1024 * 1024 * 32);
    if (mem) {
      sleep(5);
      free(mem);
      mem = NULL;
    }
    return true;
  }
};

int main(int argc, char** argv) {
  thread_pool pool(10);

  int print_task_num = 20;
  int memory_task_num = 10;
  int task_num = print_task_num + memory_task_num;
  thread_task** tasks = new thread_task*[task_num];

  pool.start();
  for (int i = 0; i < task_num; ++i) {
    if (i < print_task_num) {
      tasks[i] = new print_task(i);
    } else {
      tasks[i] = new memory_task;
    }
    pool.add_task(tasks[i]);
  }

  sleep(8);

  pool.stop();
  pool.join();

  for (int i = 0; i < task_num; ++i) {
    delete tasks[i];
  }

  delete[] tasks;
  return 0;
}
