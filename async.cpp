#include "async.h"
#include "thread_queue.h"
#include <sstream>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>

//для удобной сборки в Visual Studio
#ifdef WIN32
#include <iomanip>
#endif

namespace async {

std::mutex console_mutex;              // мьютекс для вывода в консоль
std::mutex file_mutex;                 // мьютекс для вывода в файл

std::condition_variable console_cv;
std::condition_variable file_cv;

thread_queue<std::vector<std::string>> log_queue;
thread_queue<std::vector<std::string>> file_queue;

std::thread log_thread;
std::thread file_thread1;
std::thread file_thread2;

bool threads_started = false;   // потоки запущены


//Функция для потока вывода в консоль
void logThreadFunction() {
    DLOG(" logThreadFunction started" << std::endl;)
    std::vector<std::string> output_block;  // объявляем здесь
    while(log_queue.pop(output_block)) {
        DLOG("logThreadFunction popped block, size=" << output_block.size() << std::endl;)
        print_block_to_console(output_block);
    }
    DLOG("logThreadFunction exiting" << std::endl;)
}

//Функция для потока вывода в файл
void fileThreadFunction() {
    std::vector<std::string> output_block;
    while(file_queue.pop(output_block))
    {print_block_to_file(output_block);}
}

void threads_start()
{
  //std::cerr << "[DEBUG] threads_start() called, threads_started=" << threads_started << std::endl;
 log_queue.reset();   
 file_queue.reset(); 
 if(!threads_started)
  {
    log_thread = std::thread(logThreadFunction);
    file_thread1 = std::thread(fileThreadFunction);
    file_thread2 = std::thread(fileThreadFunction); 
    threads_started = true;  
  }
}

void threads_stop()
{
  //std::cerr << "[DEBUG] threads_stop() called, threads_started=" << threads_started << std::endl;
  if(threads_started)
  {
    //std::this_thread::sleep_for(std::chrono::milliseconds(100)); // дать время на обработку
    log_queue.stop();
    file_queue.stop();
  
    log_thread.join();
    file_thread1.join();
    file_thread2.join(); 
    threads_started = false;
  }
}


//Функция формирования имени файла, статический счетчик исключает совпадение имен.
std::string generateFilename()
{
    //статический атомарный счётчик
    static std::atomic<unsigned int> counter {0};

#ifdef WIN32
    std::time_t now = std::time(nullptr);
    std::tm timeinfo;
    localtime_s( &timeinfo, &now);
#else
    // Получаем текущее время
    std::time_t now = std::time(nullptr);
    std::tm timeinfo;
    localtime_r(&now, &timeinfo);
#endif
    // Формируем имя файла
    std::string filename = "bulk";
    filename += std::to_string(timeinfo.tm_mday);
    filename += std::to_string(timeinfo.tm_mon + 1);
    filename += std::to_string(timeinfo.tm_year + 1900);
    filename += std::to_string(timeinfo.tm_hour);
    filename += std::to_string(timeinfo.tm_min);
    filename += std::to_string(timeinfo.tm_sec);

    // Счетчик увеличивается каждом вызове, гарантируя уникальность имени.
    filename += "_" + std::to_string(++counter);

    filename += ".log";

    return filename;
}


// вывод блока в консоль потоком log
void print_block_to_console(const std::vector<std::string>& cmds) {

   //std::cerr << "[DEBUG] print_block_to_console called, cmds size=" << cmds.size() << std::endl;
    if(cmds.empty()) {return;}
    std::lock_guard<std::mutex> lock(console_mutex);
    
    std::cout << "\nbulk : ";
  
    for (size_t i = 0; i < cmds.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << cmds[i];
    } 
    std::cout.flush();
    //std::cerr << "[DEBUG] print_block_to_console finished" << std::endl;
  }

// запись блока в файл файловым потоком
void print_block_to_file(const std::vector<std::string>& cmds) {
    if(cmds.empty()) {return;}
    std::string filename = generateFilename();
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file." << std::endl;
    }

    file << "bulk : ";
    
    for (size_t i = 0; i < cmds.size(); ++i) {
        if (i > 0) {
            file << ", ";
        }
        file << cmds[i];
    }
    // Файл закроется автоматически при выходе из функции (деструктор ofstream)
    //file.close();
}

BulkContext* connect(std::size_t bulk) {
    auto* ctx = new BulkContext(bulk);
    DLOG("connect created ctx=" << ctx << std::endl;)
    return ctx;
}

//добавляем сформированные блоки в очереди
void add_block_to_queues(std::vector <std::string> block)
{
    DLOG( "add_block_to_queues size=" << block.size() << std::endl;)
    // в очередь для консоли
    {
        std::lock_guard<std::mutex> lock(console_mutex);
        log_queue.push(block);
        DLOG("DEBUG: pushed to log_queue" << std::endl;)
    }
    console_cv.notify_one();
    
    //в очередь для файлов
    {
        std::lock_guard<std::mutex> lock(file_mutex);
        file_queue.push(block);
    }
    file_cv.notify_one();
    DLOG("DEBUG: add_block_to_queues EXIT" << std::endl;)
}

void receive(BulkContext* ctx, const char* data, std::size_t size) {
    if (ctx){
      //  копируем в строку size символов из data
      std::string input(data, size);
      //чтобы работать как с входным потоком
      std::istringstream stream(input);
      std::string line;
      
      while (std::getline(stream, line))  {
        ctx->process(line);
      }
    
    }

}

void disconnect(BulkContext* ctx) {
    if (ctx) {
        delete ctx;
        DLOG("сtx deleted" << std::endl);
    }
}

void BulkContext::process(std::string& cmd)
{
    DLOG("process: cmd='" << cmd << "', depth=" << depth << ", commands.size=" <<        commands.size() << std::endl;)
   if (cmd.empty()) return;
   if (cmd.back() == '\r')
   {
      cmd.pop_back();
   }
  
   if (cmd == "{")
    {
        if(depth == 0)
        {
          add_block_to_queues(commands);
          commands.clear();
        }
        depth++;       
    }
    else if (cmd == "}")
    {
          depth--;
          if(depth<0) {depth = 0;}
          if(depth == 0)
          {
            add_block_to_queues(commands);
            commands.clear();
          }
    }
    else {
        commands.push_back(cmd);
        if (depth == 0 && commands.size() == bulk_size)
        {
            add_block_to_queues(commands);
            commands.clear();
        }

    }
}

}  // namespace async





