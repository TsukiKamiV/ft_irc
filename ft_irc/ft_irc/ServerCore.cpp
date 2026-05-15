#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"

static volatile sig_atomic_t	g_running = 1;

Server::Server(long port, const std::string &password)
: _port(port),
_password(password),
_serverSocketFd(-1),
_clients(),
_channels(),
_fds()
{}

Server::~Server() {
	for (std::size_t i = 0; i < _fds.size(); ++i) {
		if (_fds[i].fd >= 0)
			close(_fds[i].fd);
	}
	_fds.clear();
}

static void	signalHandler(int signum) {
	(void)signum;
	g_running = 0;
}

void	Server::initServer() {
	std::signal(SIGINT,  signalHandler);
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGPIPE, SIG_IGN);
	
	_serverSocketFd = createListeningSocket(_port);
	if (_serverSocketFd == -1)
		throw std::runtime_error("failed to create listening socket");
	
	struct pollfd	serverPoll;
	serverPoll.fd = _serverSocketFd;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;
	_fds.push_back(serverPoll);
	
	std::cout << "[OK] server socket created." << std::endl;
	std::cout << "[OK] listening on 0.0.0.0:" << _port << std::endl;
	std::cout << "[OK] managed by poll()" << std::endl;
	std::cout << "[INFO] test: nc 127.0.0.1 " << _port << std::endl;
	std::cout << "[INFO] Press Ctrl+C to exit." << std::endl;
	std::cout << "[OK] server is ready to accept clients." << std::endl;
}

void	Server::run() {
	while (g_running != 0) {
		try {
			handlePollEvents();
		} catch (const std::exception &e) {
			std::cerr << "[ERROR] " << e.what() << std::endl;
		}
	}
}

void	Server::handlePollEvents() {
	if (_fds.empty()) {
		std::cerr << "[ERROR] poll list is empty" << std::endl;
		return;
	}
	const int	pollRet = poll(&_fds[0], _fds.size(), 3000);
	if (pollRet == -1) {
		if (errno == EINTR)
			return;
		std::cerr << "[ERROR] poll: " << std::strerror(errno) << std::endl;
		return;
	}
	if (pollRet == 0)
		return;
	
	size_t	i = 0;
	while (i < _fds.size()) {
		if (_fds[i].revents == 0) {
			++i;
			continue;
		}
		if (_fds[i].fd == _serverSocketFd) {
			if (_fds[i].revents & POLLIN)
				acceptNewClient();
			++i;
			continue;
		}
		const bool	clientRemoved = handleClientEvent(i);
		if (!clientRemoved) {
			if (i < _fds.size())
				_fds[i].revents = 0;
			++i;
		}
	}
}

void	Server::acceptNewClient() {
	struct sockaddr_in	clientAddr;
	socklen_t	clientLen = sizeof(clientAddr);
	
	const int	clientFd = accept(_serverSocketFd,
								  reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);
	if (clientFd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		throw std::runtime_error(
								 std::string("accept failed: ") + std::strerror(errno));
	}
	
	if (setNonBlocking(clientFd) == -1) {
		close(clientFd);
		throw std::runtime_error("failed to set client socket to non-blocking");
	}
	
	const std::string	clientIp = inet_ntoa(clientAddr.sin_addr);
	
	Client newClient(clientFd, clientIp);
	if (clientIp == "127.0.0.1")
		newClient.setHostField("localhost");
	else
		newClient.setHostField(clientIp);
	
	struct pollfd	clientPoll;
	clientPoll.fd = clientFd;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	
	try {
		_clients.reserve(_clients.size() + 1);
		_fds.reserve(_fds.size() + 1);
	} catch (const std::bad_alloc &) {
		close(clientFd);
		std::cerr << "[ERROR] acceptNewClient: out of memory, connection rejected" << std::endl;
		return;
	}
	
	_clients.push_back(newClient);
	_fds.push_back(clientPoll);
	
	std::cout << "[Poll] new client accepted from "
	<< clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;
	std::cout << "[OK] client fd " << clientFd << " added." << std::endl;
}

void	Server::removeClient(size_t index, const std::string &reason) {
	if (index == 0 || index >= _fds.size())
		return;
	
	const int	fd = _fds[index].fd;
	close(fd);
	
	const size_t	clientIndex = index - 1;
	const std::string	reasonStr = reason.empty() ? "Client quit" : reason;
	
	removeClientFromChannels(clientIndex, fd, reasonStr);
	
	if (clientIndex < _clients.size())
		_clients.erase(_clients.begin() + clientIndex);
	_fds.erase(_fds.begin() + index);
	
	std::cout << "[OK] client fd " << fd << " removed." << std::endl;
}

bool	Server::handleClientEvent(size_t index) {
	char	buffer[513];
	size_t	clientIndex;
	
	if (index == 0 || index >= _fds.size())
		return false;
	
	clientIndex = index - 1;
	if (clientIndex >= _clients.size()) {
		std::cerr << "[ERROR] client index out of range at poll index " << index << std::endl;
		return false;
	}
	
	const int	fd = _fds[index].fd;
	
	if (_fds[index].revents & (POLLHUP | POLLERR | POLLNVAL)) {
		removeClient(index, "Connection closed");
		return true;
	}
	if (_fds[index].revents & POLLOUT)
		handleSendBuffer(clientIndex, "");
	if (_clients[clientIndex].shouldDisconnect()) {
		removeClient(index, "Connection closed");
		return true;
	}
	if ((_fds[index].revents & POLLIN) == 0)
		return false;
	
	const int	bytesRead = recv(fd, buffer, 512, 0);
	if (bytesRead == 0) {
		std::cout << "[INFO] client fd " << fd << " disconnected." << std::endl;
		removeClient(index, "client disconnected");
		return true;
	}
	if (bytesRead == -1) {
		if (errno == EINTR)
			return false;
		std::cerr << "[ERROR] recv on fd " << fd << ": " << std::strerror(errno) << std::endl;
		removeClient(index, "recv error");
		return true;
	}
	buffer[bytesRead] = '\0';
	return handleClientBuffer(clientIndex, std::string(buffer, bytesRead));
}

int	Server::createListeningSocket(long port) {
	int	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1)
		return -1;
	
	int	opt = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		close(serverFd);
		return -1;
	}
	if (setNonBlocking(serverFd) == -1) {
		close(serverFd);
		return -1;
	}
	
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	
	if (bind(serverFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
		close(serverFd);
		return -1;
	}
	if (listen(serverFd, MAX_CONNECTION) == -1) {
		close(serverFd);
		return -1;
	}
	return serverFd;
}

int	Server::setNonBlocking(int fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return 0;
}
