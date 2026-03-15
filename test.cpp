
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
    
    // Запускаем потоки 
    async::threads_start();
    
    size_t block_size = 3;
    const char* data = "1\n2\n3\n";
    const char* expected = "\nbulk : 1, 2, 3";
  
    // Даем время потокам обработать
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto ctx = async::connect(block_size);
    async::receive(ctx, data, strlen(data));
    async::disconnect(ctx);

    // Даем время на обработку последнего блока
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    //std::cerr << "Expected: [" << expected << "]" << std::endl;
    //std::cerr << "Actual  : [" << output.str() << "]" << std::endl;
    
    BOOST_CHECK(output.is_equal(expected));

    // Восстанавливаем cout
    std::cout.rdbuf(old_cout);
    
    // Останавливаем потоки после теста
    async::threads_stop();
}

//тест для динамического блока (с фигурными скобками)
BOOST_AUTO_TEST_CASE(dynamic_blocks) {

    boost::test_tools::output_test_stream output;
    std::streambuf* old_cout;
    
    // Перехватываем std::cout
    old_cout = std::cout.rdbuf(output.rdbuf());
  
    // Запускаем потоки 
    async::threads_start();
    
    // Даем время потокам обработать
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    size_t block_size = 3;
    const char* data = "{\n5\n6\n}";
    const char* expected = "\nbulk : 5, 6";
    
    auto ctx = async::connect(block_size);
    async::receive(ctx, data, strlen(data));
    async::disconnect(ctx);
    
    // Даем время на обработку последнего блока
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Проверяем консольный вывод
    BOOST_CHECK(output.is_equal(expected));

    // Восстанавливаем cout
    std::cout.rdbuf(old_cout);
    
    // Останавливаем потоки после теста
    async::threads_stop();
}
