#pragma once


#include <condition_variable>
#include <string>
#include <queue>
#include <atomic>
#include <mutex>

/* Реализация потокобезопасной очереди. Операции добавления и вставки
push() и pop() выполняются при залоченном мьютексе. */

template<typename T>
class thread_queue
{
  std::queue<T> queue_;           // формируемая очередь
  std::mutex mutex_;              // мьютекс для защиты доступа к очереди
  std::condition_variable cv_;    // условая переменная - поток будет ждать данные
  bool done_ = false;             // флаг для завершения ожидания

  public:
      //добавить элемент в очереди, предварительно ее заблокировав
      void push(const T& value)
      {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        cv_.notify_one();
      }
      //достать элемент из очереди, предварительно ее заблокировав
      bool pop(T& value)
      {
        std::unique_lock<std::mutex> lock(mutex_);
        // ждем появления данных, пока очередь пустая - спим.
        cv_.wait(lock, [this] {return !queue_.empty() || done_; });
        if(queue_.empty()) {return false;}
        value = queue_.front();
        queue_.pop();
        return true;
      }
      void stop()
      {
        std::unique_lock<std::mutex> lock(mutex_); 
        while(!queue_.empty()) queue_.pop();
        
        done_ = true;
        cv_.notify_all();
      }
      
      void reset() {
      std::unique_lock<std::mutex> lock(mutex_);
      while(!queue_.empty()) queue_.pop();
      done_ = false;
}
		
	
};








