#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "debug_settings.h"
#include "async.h"

using boost::asio::ip::tcp;

extern std::size_t g_bulk_size;
extern boost::asio::io_context io_context;
extern std::unique_ptr<tcp::acceptor> acceptor;
extern std::atomic_uint connection_id;
extern unsigned short g_port; 


// Класс, представляющий одно подключение
struct Connection : public std::enable_shared_from_this<Connection>
{
   tcp::socket socket;
   async::BulkContext* ctx_;
   boost::asio::streambuf streambuf_;
   
   void start_read();
   
   static std::shared_ptr<Connection> Create(boost::asio::io_context& io_context);
   
   Connection(boost::asio::io_context& io_context);
   ~Connection();
};

// Функции
void BeginAcceptConnection();
void HandleAccept(std::shared_ptr<Connection> connection, const boost::system::error_code& err);
void ProcessConnection(std::shared_ptr<Connection> c);
void HandleWrite(const boost::system::error_code& err, size_t bytes_send);
