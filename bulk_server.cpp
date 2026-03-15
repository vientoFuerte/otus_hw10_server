#include "bulk_server.h"
#include <iostream>
#include <sstream>
#include <string>

// Глобальные переменные (определения)
std::size_t g_bulk_size = 0;
boost::asio::io_context io_context;
std::unique_ptr<tcp::acceptor> acceptor;
std::atomic_uint connection_id = 0;
unsigned short g_port; 


static std::string message = "TCP/IP Hello world\n";
static std::string message_hash = "\n1234\n";

// Реализация методов
Connection::Connection(boost::asio::io_context& io_context) : socket(io_context)
{
    ctx_ = async::connect(g_bulk_size);
}

Connection::~Connection() 
{
    async::disconnect(ctx_);
}

std::shared_ptr<Connection> Connection::Create(boost::asio::io_context& io_context)
{
    return std::make_shared<Connection>(io_context);
}

void Connection::start_read() 
{
    auto self = shared_from_this();
    boost::asio::async_read_until(socket, streambuf_, '\n',
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (!ec) {
                std::istream is(&streambuf_);
                std::string line;
                while(std::getline(is, line))
                {
                    async::receive(ctx_, line.c_str(), line.size());
                }
                start_read();
            } else {
                if (streambuf_.size() > 0) {
                    std::cout << "=== BUFFER CONTENTS (" << streambuf_.size() << " bytes) ===" << std::endl;
                    std::string remaining(boost::asio::buffers_begin(streambuf_.data()),
                                          boost::asio::buffers_begin(streambuf_.data()) + streambuf_.size());
                    streambuf_.consume(streambuf_.size());
                    
                    std::istringstream is(remaining);
                    std::string line;
                    while(std::getline(is, line))
                    {
                        async::receive(ctx_, line.c_str(), line.size());
                    }
                }
                DLOG("Connection error, calling disconnect for ctx=" << ctx_ << std::endl);
                async::disconnect(ctx_);
                ctx_ = nullptr;
            }
        });
}

// Реализация функций
void BeginAcceptConnection()
{
    std::shared_ptr<Connection> c = Connection::Create(io_context);
    acceptor->async_accept(c->socket,
        [c](const boost::system::error_code& err) {
            HandleAccept(c, err);
        });   
}

void HandleAccept(std::shared_ptr<Connection> connection, const boost::system::error_code& err)
{
    if (err) {
        std::cout << err.message() << std::endl;
        BeginAcceptConnection();
    }
    else {
        connection_id++;
        ProcessConnection(connection);
        BeginAcceptConnection();
    } 
}

void ProcessConnection(std::shared_ptr<Connection> c)
{
    auto msg = std::make_shared<std::string>(
        message + " " + std::to_string(connection_id) + message_hash
    );
    
    boost::asio::async_write(
        c->socket,
        boost::asio::buffer(*msg),
        [c, msg](const boost::system::error_code& err, size_t bytes_send) {
            if(!err)
            {
                c->start_read();
            }
        }
    );
}

void HandleWrite(const boost::system::error_code& err, size_t bytes_send)
{
    (void)err;
    std::cout << "SEND:" << bytes_send << std::endl;
}
