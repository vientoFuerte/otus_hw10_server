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
// Глобальная переменная для порта
unsigned short g_port = 0;
// IO контекст и acceptor (теперь будет создаваться после получения порта)
boost::asio::io_context io_context;
std::unique_ptr<tcp::acceptor> acceptor;  // Используем unique_ptr, так как acceptor будет создан позже

// Класс, представляющий одно подключение
struct Connection : public std::enable_shared_from_this<Connection>
{    

   tcp::socket socket;
  // std::size_t bulk_size_;
   async::BulkContext* ctx_;
   boost::asio::streambuf streambuf_;
   
   void start_read();
   
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

void Connection::start_read() {
    auto self = shared_from_this();
    boost::asio::async_read_until(socket, streambuf_, '\n',
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (!ec) {
                std::istream is(&streambuf_);
                std::string line;
                std::getline(is, line);
                // Передаём команду в библиотеку async
                async::receive(ctx_, line.c_str(), line.size());
                // Продолжаем чтение
                start_read();
            } else {
                // Ошибка или клиент отключился – соединение закроется автоматически
                // (деструктор Connection вызовет async::disconnect)
            }
        });
}


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
    acceptor->async_accept(c->socket,
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
        [c, msg](const boost::system::error_code& err, size_t bytes_send) {
            //HandleWrite(err, bytes_send);
            if(!err)
            {
                c->start_read();  // начинаем читать команды после отправки
            }
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
       
         // После остановки io_context останавливаем потоки
         async::threads_stop();
              
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
