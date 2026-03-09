
#define BOOST_TEST_MODULE AcyncServerTests
#include <boost/test/included/unit_test.hpp>
#include <fstream>
#include "async.h"
#include "bulk_server.h"
#include <thread>

//тест для статического блока (без фигурных скобок)
BOOST_AUTO_TEST_CASE(static_blocks) {

    boost::test_tools::output_test_stream output;
    std::streambuf* old_cout;
    
    // Перехватываем std::cout
    old_cout = std::cout.rdbuf(output.rdbuf());
    
    /*size_t block_size = 3;
    const char* data = "cmd1\ncmd2\ncmd3\n";
    const char* expected = "\nbulk : cmd1, cmd2, cmd3";
  
    auto ctx = async::connect(block_size);
    async::receive(ctx, data, strlen(data));
    async::disconnect(ctx);

    BOOST_CHECK(output.is_equal(expected));*/

    // Восстанавливаем cout
    std::cout.rdbuf(old_cout);
}

//тест для динамического блока (с фигурными скобками)
BOOST_AUTO_TEST_CASE(dynamic_blocks) {

    boost::test_tools::output_test_stream output;
    std::streambuf* old_cout;
    
    // Перехватываем std::cout
    old_cout = std::cout.rdbuf(output.rdbuf());
  
    /*size_t block_size = 3;
    const char* data = "{\ncmd5\ncmd6\n}";
    const char* expected = "\nbulk : cmd5, cmd6";
    
    auto ctx = async::connect(block_size);
    async::receive(ctx, data, strlen(data));
    async::disconnect(ctx);*/

   // Проверяем консольный вывод
   //BOOST_CHECK(output.is_equal(expected));

    // Восстанавливаем cout
    std::cout.rdbuf(old_cout);
}
