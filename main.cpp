// boost_visualcpp_template.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <ctime>
#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include "bulk_server.h"


int main(int argc, char* argv[])
{

    if (argc != 3) {
            std::cerr << "Usage: bulk_server <port> <bulk_size>\n";
            return 1;
    
    }
    
    //std::cout << "SERVER PROGRAM" << std::endl;
    try
    {
        g_port = static_cast<unsigned short>(std::stoi(argv[1]));     // порт
        g_bulk_size = static_cast<std::size_t>(std::stoll(argv[2]));  // размер блока      
       
        // Запускаем потоки вывода один раз
        async::threads_start();
        
        // Создаем acceptor с переданным портом
        acceptor = std::make_unique<tcp::acceptor>(
          io_context, 
          tcp::endpoint(tcp::v4(), g_port)
         );
         BeginAcceptConnection();

         io_context.run(); // блокируется до завершения
       
         // Даем время потокам вывода обработать последние блоки
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         
         // После остановки io_context останавливаем потоки
         async::threads_stop();
              
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
