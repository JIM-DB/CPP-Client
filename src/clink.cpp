﻿#include "clink.h"
#include "log/logger.h"

#define MESSAGE_SIZE 8
namespace jimdb
{
    CLink::CLink(asio::io_service& service, const std::string& host, const std::string& port,
                 const int& i) : m_socket(service),
        m_timeout(i), m_connected(false)
    {
        asio::ip::tcp::resolver l_res(service);
        asio::ip::tcp::resolver::query l_query(asio::ip::tcp::v4(), host, port);
        asio::async_connect(m_socket, l_res.resolve(l_query),
                            [&](std::error_code ec, asio::ip::tcp::resolver::iterator it)
        {
            if (ec)
                LOG_ERROR << ec.message();//  throw std::runtime_error(ec.message());
        });
        await_operation(std::chrono::milliseconds(m_timeout));
    }

    CLink::~CLink()
    {
        // m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
        // m_socket.close();
    }

    void CLink::operator>>(std::shared_ptr<std::string> get)
    {
        auto l_buffer = read(MESSAGE_SIZE);
        if (l_buffer == nullptr)
        {
            get = nullptr;
            return;
        }

        //never use atoi
        auto l_size = 0;
        std::stringstream ss;
        ss << l_buffer;
        ss >> l_size;

        delete[] l_buffer;

        if (l_size == 0)
        {
            get = nullptr;
            return;
        }

        l_buffer = read(l_size);
        if (l_buffer != nullptr)
        {
            get->append(l_buffer);
            delete[] l_buffer;
            return;
        }

        get = nullptr;
        return;
    }

    void CLink::operator<<(std::shared_ptr<std::string> tosend)
    {
        if (tosend == nullptr)
            return;
        char length[MESSAGE_SIZE + 1];
        sprintf(length, "%8d", static_cast<int>(tosend->size()));
        auto l_message = std::string(length);
        l_message += *tosend;

        //this doesnt like string itself getting xstring issues so go for the cstring.
        m_socket.async_write_some(asio::buffer(l_message.c_str(), l_message.size()), [&](asio::error_code ec,
                                  size_t bytes_read)
        {
            if (ec)
                LOG_DEBUG << ec.message();
        });
		m_socket.get_io_service().run_one();
        await_operation(std::chrono::seconds(1));
    }

    char* CLink::read(const size_t& count)
    {
        auto l_buffer = new char[count + 1];
        l_buffer[count] = '\0';
        asio::async_read(m_socket, asio::buffer(l_buffer, count), [&](std::error_code ec, size_t bytes_read)
        {
            if (ec)
                LOG_ERROR << ec.message();
        });
        //wait for the task to finish
        await_operation(std::chrono::milliseconds(m_timeout));
        return l_buffer;
    }
}