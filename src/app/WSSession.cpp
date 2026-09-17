#include "WSSession.hpp"

namespace binagg
{

    void fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }

    void WSSession::run(std::string host, std::string port, std::string target, std::function<void(std::string_view)> on_message)
    {
        host_ = std::move(host);
        target_ = std::move(target);
        on_message_ = on_message;

        ws_.next_layer().set_verify_callback(ssl::host_name_verification(host_.c_str()));

        resolver_.async_resolve(host_, port,
            beast::bind_front_handler(&WSSession::on_resolve, shared_from_this()));
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

        std::cout << "Connected to Binance! Streaming trades...\n";

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
            return;

        if (ec)
            return fail(ec, "read");

        on_message_(std::string_view(static_cast<const char*>(buffer_.data().data()), buffer_.data().size()));

        buffer_.consume(buffer_.size());
        do_read();
    }

    void WSSession::on_close(beast::error_code ec)
    {
        if (ec) return fail(ec, "close");
        on_message_(std::string_view(static_cast<const char*>(buffer_.data().data()), buffer_.data().size()));
    }

} // namespace binagg
