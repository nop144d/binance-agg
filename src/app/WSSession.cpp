#include "WSSession.hpp"

namespace binagg
{

	void WSSession::run(std::string host, std::string port, std::string target,
		MessageHandler on_message, CloseHandler on_close)
	{
		host_ = std::move(host);
		target_ = std::move(target);
		on_message_ = std::move(on_message);
		on_close_ = std::move(on_close);

		ws_.next_layer().set_verify_callback(ssl::host_name_verification(host_.c_str()));

		resolver_.async_resolve(host_, port,
			beast::bind_front_handler(&WSSession::on_resolve, shared_from_this()));
	}

	void WSSession::close()
	{
		// Hop onto the stream's strand so the flags below are only ever touched
		// from one place, whoever calls this.
		net::post(ws_.get_executor(), [self = shared_from_this()] { self->do_close(); });
	}

	void WSSession::do_close()
	{
		if (closing_ || finished_) {
			return;
		}
		closing_ = true;

		if (connected_) {
			ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(&WSSession::on_close, shared_from_this()));
			return;
		}

		// Nothing to close yet: abort whatever stage of setup is in flight.
		resolver_.cancel();
		beast::get_lowest_layer(ws_).cancel();
	}

	void WSSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
	{
		if (ec)
			return fail(ec, "resolve");

		beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

		beast::get_lowest_layer(ws_).async_connect(results,
			beast::bind_front_handler(&WSSession::on_connect, shared_from_this()));
	}

	void WSSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
	{
		boost::ignore_unused(ep);

		if (ec)
			return fail(ec, "connect");

		beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

		if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
			ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
			return fail(ec, "ssl_sni");
		}

		ws_.next_layer().async_handshake(ssl::stream_base::client,
			beast::bind_front_handler(&WSSession::on_ssl_handshake, shared_from_this()));
	}

	void WSSession::on_ssl_handshake(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "ssl_handshake");

		beast::get_lowest_layer(ws_).expires_never();

		ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

		ws_.set_option(websocket::stream_base::decorator(
			[](websocket::request_type& req)
			{
				req.set(http::field::user_agent,
					std::string(BOOST_BEAST_VERSION_STRING) +
					" websocket-client-async-ssl");
			}));

		ws_.async_handshake(host_, target_,
			beast::bind_front_handler(&WSSession::on_handshake, shared_from_this()));
	}

	void WSSession::on_handshake(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "handshake");

		connected_ = true;
		spdlog::info("connected, streaming trades");

		if (closing_) {
			// close() arrived while the handshake was still in flight.
			closing_ = false;
			return do_close();
		}

		do_read();
	}

	void WSSession::do_read()
	{
		ws_.async_read(buffer_,
			beast::bind_front_handler(&WSSession::on_read, shared_from_this()));
	}

	void WSSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec == websocket::error::closed)
			return finish({});

		if (ec)
			return fail(ec, "read");

		on_message_(std::string_view(static_cast<const char*>(buffer_.data().data()), buffer_.data().size()));
		buffer_.consume(buffer_.size());

		if (!finished_) {
			do_read();
		}
	}

	void WSSession::on_close(beast::error_code ec)
	{
		if (ec) {
			// Binance drops the TCP connection without a TLS shutdown, which shows
			// up here as "stream truncated". Harmless during a close we asked for.
			spdlog::debug("close: {}", ec.message());
		}
		finish({});
	}

	void WSSession::fail(beast::error_code ec, char const* what)
	{
		if (closing_) {
			// operation_aborted and friends are expected while shutting down.
			return finish({});
		}

		spdlog::error("{}: {}", what, ec.message());
		finish(ec);
	}

	void WSSession::finish(beast::error_code ec)
	{
		if (finished_) {
			return;
		}
		finished_ = true;

		if (on_close_) {
			on_close_(ec);
		}
	}

} // namespace binagg
