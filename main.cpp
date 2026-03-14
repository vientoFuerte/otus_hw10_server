// boost_visualcpp_template.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <memory>
#include <atomic>
#include "async.h"


using boost::asio::ip::tcp;

std::string message = "TCP/IP Hello world\n";

std::string message_hash = "1234";
std::atomic_uint connection_id = 0;
// Глобальная переменная для размера блока
std::size_t g_bulk_size = 0;

// IO контекст и acceptor (прослушивает порт 18000)
boost::asio::io_context io_context;
tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 18000));



// Класс, представляющий одно подключение
struct Connection : public std::enable_shared_from_this<Connection>
{    

   tcp::socket socket;
  // std::size_t bulk_size_;
   async::BulkContext* ctx_;
   
   static std::shared_ptr<Connection> Create(boost::asio::io_context& io_context)
   {
      return std::make_shared<Connection>(io_context);
   }


    Connection(boost::asio::io_context& io_context) : socket(io_context)
    {
        ctx_ = async::connect(g_bulk_size);
    }
    
      ~Connection() {
         async::disconnect(ctx_);  // при разрушении сессии удаляем контекст
    }
};


void HandleAccept(std::shared_ptr<Connection> connection,const boost::system::error_code& err);
void HandleWrite(const boost::system::error_code& err, size_t bytes_send);
void ProcessConnection(std::shared_ptr<Connection>);

void BeginAcceptConnection()
{
    
    std::shared_ptr<Connection> c = Connection::Create(io_context);
    //std::shared_ptr<tcp::socket> s = new tcp::socket(io_context);
    //acceptor.async_accept(c->socket, std::bind(HandleAccept,c,boost::asio::placeholders::error));
    // Асинхронно ждём подключения.
    // Вместо std::bind используем лямбда-захват shared_ptr c.
    acceptor.async_accept(c->socket,
        [c](const boost::system::error_code& err) {
            HandleAccept(c, err);
        });
    
    
}

void HandleAccept(std::shared_ptr<Connection> connection, const boost::system::error_code& err)
{
    
    std::cout << connection_id << std::endl;
    
    if (err) {
        std::cout << err.message() << std::endl;
        BeginAcceptConnection();
    }
    else {
        connection_id++;
        std::cout << "CONNECTION ACCEPTED:" << std::endl;
        
        // Обрабатываем подключение (отправляем приветствие)
        ProcessConnection(connection);
        
        // Снова начинаем ждать следующее подключение
        BeginAcceptConnection();
    }
       

    
}

void ProcessConnection(std::shared_ptr<Connection> c)
{
    // Используем shared_ptr, чтобы данные жили до завершения записи
    auto msg = std::make_shared<std::string>(
        message + " " + std::to_string(connection_id) + message_hash
    );
    
    boost::asio::async_write(
        c->socket,
        boost::asio::buffer(*msg),
        [msg](const boost::system::error_code& err, size_t bytes_send) {
            HandleWrite(err, bytes_send);
        }
    );
}



void HandleWrite(const boost::system::error_code& err,size_t bytes_send)
{
    (void)err; // убирает предупреждение о неиспользуемом параметре
    std::cout << "SEND:" << bytes_send << std::endl;
}


int main(int argc, char* argv[])
{

    if (argc != 3) {
            std::cerr << "Usage: bulk_server <port> <bulk_size>\n";
            return 1;
    
    }
    
    std::cout << "SERVER PROGRAM" << std::endl;
    try
    {
               
        BeginAcceptConnection();

        io_context.run();
              
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

