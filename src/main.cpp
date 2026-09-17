#include <iostream>

#include "app/WSSession.hpp"

using namespace binagg;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}
	else {
		std::cout << "Config file: " << argv[1] << std::endl;
	}

    ssl::context ctx{ ssl::context::tlsv13_client };
	ctx.set_verify_mode(ssl::verify_none); // Disable certificate verification for testing purposes
    ctx.set_default_verify_paths();

    std::string const host{"stream.binance.com"};
    std::string const port{"9443"};
    std::string const target{"/stream?streams=btcusdt@trade"};

    net::io_context ioc;
    std::make_shared<WSSession>(ioc, ctx)->run(std::move(host), std::move(port), std::move(target));

    ioc.run();

    return 0;
}
