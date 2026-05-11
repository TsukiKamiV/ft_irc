#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

#include <cstdlib>
#include <climits>
#include <cerrno>
#include <cctype>
#include <iostream>

static bool	isAllDigits(const char *str) {
	size_t i;
	if (str == NULL || *str == '\0')
		return false;
	i = 0;
	while (str[i] != '\0') {
		if (!std::isdigit(static_cast<unsigned char>(str[i])))
			return false;
		i++;
	}
	return true;
}

static bool	isValidPort(const char *portStr) {
	char *end;
	long port;
	
	if (!isAllDigits(portStr))
		return false;
	errno = 0;
	end = NULL;
	port = std::strtol(portStr, &end, 10);
	if (errno == ERANGE || end == NULL || *end != '\0')
		return false;
	if (port < 1 || port > 65535)
		return false;
	return true;
}

static bool	isValidPassword(const char *password) {
	if (password == NULL || password[0] == '\0')
		return false;
	int i = 0;
	while (password[i] != '\0') {
		if (password[i] == ' ' || password[i] == '\t'
			|| password[i] == '\r' || password[i] == '\n')
			return false;
		i++;
	}
	return true;
}

static bool	checkParams(int argc, const char *argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return false;
	}
	if (!isValidPort(argv[1])) {
		std::cerr << "Error: invalid port. Use a number between 1 and 65535." << std::endl;
		return false;
	}
	if (!isValidPassword(argv[2])) {
		std::cerr << "Error: invalid password." << std::endl;
		return false;
	}
	return true;
}

int main(int argc, const char * argv[]) {
	if (checkParams(argc, argv) == false)
		return EXIT_FAILURE;
	try {
		Server server(std::strtol(argv[1], NULL, 10), argv[2]);
		server.initServer();
		server.run();
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
