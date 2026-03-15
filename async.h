#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "debug_settings.h"

namespace async {
//добавляем сформированные блоки в очереди
void add_block_to_queues(std::vector <std::string> block);
void print_block_to_console(const std::vector<std::string>& cmds);
void print_block_to_file(const std::vector<std::string>& cmds);

struct BulkContext {
    std::size_t bulk_size;
    std::vector<std::string> commands;
    int depth = 0;

    BulkContext(std::size_t size) : bulk_size(size) {}
    
    ~BulkContext() {
    
      DLOG("BulkContext destructor START, depth=" << depth 
              << ", commands.size=" << commands.size() << std::endl;) 
      // При разрушении выводим накопленные команды,
      // только если не находимся внутри динамического блока.
      if (depth == 0 && !commands.empty()) {
          DLOG("destructor adding block of size " << commands.size() << std::endl);
          add_block_to_queues(commands);
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
