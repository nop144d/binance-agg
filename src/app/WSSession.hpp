#pragma once

// based on the Boost.Beast WebSocket SSL client example:
// https://github.com/boostorg/beast/blob/develop/example/websocket/client/async-ssl/websocket_client_async_ssl.cpp


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
		using MessageHandler = std::function<void(std::string_view)>;

		// Called exactly once when the session ends. The code is empty for a
		// close we asked for, and carries the failure otherwise.
		using CloseHandler = std::function<void(beast::error_code)>;

		explicit WSSession(net::io_context& ioc, ssl::context& ctx)
			: resolver_(net::make_strand(ioc))
			, ws_(net::make_strand(ioc), ctx)
		{
		}

		void run(std::string host, std::string port, std::string target,
			MessageHandler on_message, CloseHandler on_close);

		// Starts a graceful close, or cancels a connection still being set up.
		// Safe to call more than once and from outside the strand.
		void close();

	private:
		void do_close();
		void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
		void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
		void on_ssl_handshake(beast::error_code ec);
		void on_handshake(beast::error_code ec);
		void do_read();
		void on_read(beast::error_code ec, std::size_t bytes_transferred);
		void on_close(beast::error_code ec);
		void fail(beast::error_code ec, char const* what);
		void finish(beast::error_code ec);

		tcp::resolver resolver_;
		websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
		beast::flat_buffer buffer_;
		std::string host_;
		std::string target_;
		MessageHandler on_message_;
		CloseHandler on_close_;
		bool connected_{ false };
		bool closing_{ false };
		bool finished_{ false };
	};

} // namespace binagg
