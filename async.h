#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <memory>       //  для std::shared_ptr
#include "debug_settings.h"

namespace async {

//добавляем сформированные блоки в очереди
void add_block_to_queues(std::vector <std::string> block);
void print_block_to_console(const std::vector<std::string>& cmds);
void print_block_to_file(const std::vector<std::string>& cmds);

// Глобальный контекст для накопления команд между соединениями
struct GlobalContext {
    std::vector<std::string> wait_commands;
    std::mutex mutex;
    std::size_t bulk_size;
    GlobalContext(std::size_t size): bulk_size (size) {};
    
    void add_command(const std::string& cmd)
    {
      std::lock_guard <std::mutex> lock(mutex);
      wait_commands.push_back(cmd);
      if(wait_commands.size() >= bulk_size)
      {
          add_block_to_queues(wait_commands);
          wait_commands.clear();
      }    
    }
    void flush() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!wait_commands.empty()) {
            add_block_to_queues(wait_commands);
            wait_commands.clear();
        }
    }
};

struct BulkContext {
    std::size_t bulk_size;
    std::vector<std::string> commands;
    int depth = 0;
    std::shared_ptr<GlobalContext> global_ctx; // ссылка на глобальный контекст

    BulkContext(std::size_t size, std::shared_ptr<GlobalContext> global) : bulk_size(size), global_ctx(global) {}
    
    ~BulkContext() {
    
      DLOG("BulkContext destructor START, depth=" << depth 
              << ", commands.size=" << commands.size() << std::endl;) 
      // При разрушении выводим накопленные команды,
      // только если не находимся внутри динамического блока.
      if (depth == 0 && !commands.empty()) {
          DLOG("destructor adding block of size " << commands.size() << std::endl);
          add_block_to_queues(commands);
      }
      
     if (depth == 0 && global_ctx) {
        global_ctx->flush();
     }
      
      DLOG("BulkContext destructor END" << std::endl;)
    }

    void process(std::string& line);
};



BulkContext * connect(std::size_t bulk);
void receive(BulkContext * ctx, const char* data, std::size_t size);
void disconnect(BulkContext * ctx);

void threads_start();
void threads_stop();


};
