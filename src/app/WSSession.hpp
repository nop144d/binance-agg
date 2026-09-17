#pragma once

// based on the Boost.Beast WebSocket SSL client example:
// https://github.com/boostorg/beast/blob/develop/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <string>
#include <string_view>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace binagg
{

    class WSSession : public std::enable_shared_from_this<WSSession>
    {
    public:
        explicit WSSession(net::io_context& ioc, ssl::context& ctx)
            : resolver_(net::make_strand(ioc))
            , ws_(net::make_strand(ioc), ctx)
        {
        }

        void run(std::string host, std::string port, std::string target, std::function<void(std::string_view)> on_message);

    private:
        void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
        void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
        void on_ssl_handshake(beast::error_code ec);
        void on_handshake(beast::error_code ec);
        void do_read();
        void on_read(beast::error_code ec, std::size_t bytes_transferred);
        void on_close(beast::error_code ec);

        tcp::resolver resolver_;
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
        beast::flat_buffer buffer_;
        std::string host_;
        std::string target_;
		std::function<void(std::string_view)> on_message_;
    };

    void fail(beast::error_code ec, char const* what);

} // namespace binagg
