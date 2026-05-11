#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

static bool	g_running = true;
bool	Server::_signal = false;

Server::Server(long port, const std::string &password)
: _port(port),
_password(password),
_serverSocketFd(-1),
_clients(),
_channels(),
_fds() {}
Server::~Server() {}

/**
 * Server::run()
 * └── Server::handlePollEvents()
 *     ├── Monitor file descriptors for incoming events
 *     │   └── Server::acceptNewClient()
 *     │
 *     └── Client socket has activity
 *         └── Server::handleClientEvent(index)
 *             ├── Check POLLHUP / POLLERR / POLLNVAL
 *             ├── recv(...)
 *             ├── bytesRead == 0 -> removeClient()
 *             ├── bytesRead == -1 -> handle recv error
 *             └── Server::handleClientBuffer(clientIndex, chunk)
 *                 ├── Client::appendBuffer(chunk)
 *                 ├── Search for '\n' inside Client::_buffer
 *                 ├── Extract complete IRC lines
 *                 ├── Remove trailing '\r'
 *                 └── Process the parsed line
 */

static void	sigintHandler(int signum);

void	Server::initServer() {
	struct pollfd 	serverPoll;
	
	std::signal(SIGINT, sigintHandler);
	std::signal(SIGPIPE, SIG_IGN);
	
	_serverSocketFd = Server::createListeningSocket(this->_port);
	if (_serverSocketFd == -1)
		throw std::runtime_error("failed to create listening socket");
	serverPoll.fd = _serverSocketFd;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;
	std::cout << "[OK] server socket created." << std::endl;
	std::cout << "[OK] listening on 0.0.0.0:" << _port << std::endl;
	std::cout << "[OK] now managed by poll()" << std::endl;
	std::cout << "[INFO] Available test command: nc 127.0.0.1 " << _port << std::endl;
	std::cout << "[INFO] Press Ctrl + C to exit." << std::endl;
	_fds.push_back(serverPoll);
	std::cout << "[OK] server is ready to accept and manage clients." << std::endl;
}

/**
 * @brief Create and initialize the server listening socket.
 *
 * This function performs the basic setup required for the server socket:
 * 1. Create an IPv4 / TCP socket;
 * 2. Enable address reuse with `SO_REUSEADDR`;
 * 3. Set the socket to non-blocking mode;
 * 4. Bind the socket to the given port;
 * 5. Start listening for incoming client connections.
 *
 * @param port The port number used by the server, expected to be between
 *             1 and 65535.
 * @return The listening socket fd on success, or -1 on failure.
 */

/**
	 * Socket setup overview:
	 *
	 * 1. Create an IPv4 TCP listening socket with socket(AF_INET,
	 *    SOCK_STREAM, 0).
	 *
	 * 2. Enable SO_REUSEADDR so the server can quickly reuse the same
	 *    address and port after restarting.
	 *
	 * 3. Set the socket to non-blocking mode to avoid blocking poll().
	 *
	 * 4. Configure sockaddr_in:
	 *    - AF_INET      -> IPv4 address family
	 *    - INADDR_ANY   -> listen on all available network interfaces
	 *    - htons(port)  -> convert port to network byte order
	 *
	 * 5. Bind the socket to the configured address and port.
	 *
	 * 6. Start listening for incoming connections.
	 *    The backlog value defines the maximum number of pending
	 *    connections waiting to be accepted.
	 */

int Server::createListeningSocket(long port) {
	int	serverFd;
	int	opt;
	struct sockaddr_in addr;

	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1)
		return -1;
	opt = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		close(serverFd);
		return -1;
	}
	if (setNonBlocking(serverFd) == -1) {
		close(serverFd);
		return -1;
	}
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
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

/**
 * @brief Set a socket fd to non-blocking mode.
 *
 * In non-blocking mode, socket operations such as accept(), recv(),
 * and send() return immediately instead of waiting until they can be
 * completed. If the operation would block, the call fails with errno set
 * to EAGAIN or EWOULDBLOCK, allowing the server to keep running and let
 * poll() decide when the fd should be handled again.
 */
int	Server::setNonBlocking(int fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return 0;
}

static void	sigintHandler(int signum) {
	(void)signum;
	g_running = false;
}

void	Server::run() {
	while (g_running) {
		handlePollEvents();
	}
}

/**
 * @brief Poll all managed sockets and dispatch ready events.
 *
 * This function is the main event loop step of the server. It waits for
 * activity on the listening socket and all connected client sockets.
 * When the listening socket becomes readable, a new client is accepted.
 * When a client socket has events, the event is delegated to
 * handleClientEvent().
 *
 * The function uses the index mapping between _fds and _clients:
 * _fds[0] is always the server listening socket, while _fds[i]
 * corresponds to _clients[i - 1] for connected clients.
 *
 * If handleClientEvent() removes a client, this function does not
 * increment the index, because the next pollfd entry has shifted into
 * the current position.
 */
void	Server::handlePollEvents() {
	int 	pollRet;
	size_t	i;
	bool	clientRemoved;
	
	if (_fds.empty())
		throw std::runtime_error("poll list is empty");
	pollRet = poll(&_fds[0], _fds.size(), 3000);
	if (pollRet == -1) {
		if (errno == EINTR)
			return ;
		throw std::runtime_error(std::string("Error: poll failed.") + std::strerror(errno));
	}
	if (pollRet == 0)
		return ;
	i = 0;
	while (i < _fds.size()) {
		if (_fds[i].revents == 0) {
			i++;
			continue;
		}
		if (_fds[i].fd == _serverSocketFd) {
			if (_fds[i].revents & POLLIN)
				acceptNewClient();
			i++;
			continue;
		}
		clientRemoved = handleClientEvent(i);
		if (!clientRemoved) {
			if (i < _fds.size() && _fds[i].revents != 0)
				_fds[i].revents = 0;
			i++;
		}
	}
}

/**
 * @brief Accept a new client connection and add it to server management.
 *
 * This function accepts one pending connection from the listening socket,
 * sets the new client socket to non-blocking mode, creates the matching
 * pollfd entry, stores the client information, and records the host field
 * used later in IRC message prefixes.
 */
void	Server::acceptNewClient() {
	int					clientFd;
	struct sockaddr_in 	clientAddr;
	socklen_t			clientLen;
	struct pollfd		clientPoll;
	std::string			clientIp;
	
	clientLen = sizeof(clientAddr);
	clientFd = accept(_serverSocketFd, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);
	if (clientFd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		throw std::runtime_error(std::string("Error: accept failed: ") + std::strerror(errno));
	}
	if (setNonBlocking(clientFd) == -1) {
		close(clientFd);
		throw std::runtime_error("Failed to set client socket to non-blocking");
	}
	clientPoll.fd = clientFd;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	clientIp = inet_ntoa(clientAddr.sin_addr);
	_clients.push_back(Client(clientFd, clientIp));
	_fds.push_back(clientPoll);
	if (clientIp == "127.0.0.1")
		_clients.back().setHostField("localhost");
	else
		_clients.back().setHostField(clientIp);
	std::cout << "[Poll] new client accepted from " << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;
	std::cout << "[OK] client fd " << clientFd << " added to server management." << std::endl;
}

/**
 * @brief Close and remove a client from all server-managed structures.
 *
 * The given index refers to the client's position inside _fds, not inside
 * _clients. Because _fds[0] is reserved for the server listening socket,
 * the matching client index is calculated as index - 1.
 *
 * This function closes the client socket fd, removes the client from all
 * joined channels, broadcasts the quit reason to related channel members,
 * erases the Client object from _clients, and erases the matching pollfd
 * entry from _fds.
 *
 * @param index The pollfd index of the client to remove.
 * @param reason The quit or disconnect reason used for channel broadcast.
 */
void	Server::removeClient(size_t index, const std::string &reason) {
	size_t	clientIndex;
	int		fd;
	std::string reasonStr;
	
	if (index == 0 || index >= _fds.size())
		return ;
	fd = _fds[index].fd;
	close(fd);
	clientIndex = index - 1;
	if (reason.empty())
		reasonStr = "Client quit";
	else
		reasonStr = reason;
	removeClientFromChannels(clientIndex, fd, reasonStr);
	if (clientIndex < _clients.size())
		_clients.erase(_clients.begin() + clientIndex);
	_fds.erase(_fds.begin() + index);
	std::cout << "[OK] client fd " << fd << " removed from server management." << std::endl;
}

/**
 * @brief Handle one poll event triggered by a client socket.
 *
 * This function checks socket error events first, then tries to flush
 * pending output data when POLLOUT is set. When POLLIN is set, it reads
 * available client data with recv() and forwards the received chunk to
 * the IRC line buffer parser.
 *
 * On a non-blocking socket, recv() may return -1 with EAGAIN or
 * EWOULDBLOCK. Both mean that the operation would block because no data
 * is currently available, so this is not treated as a fatal error.
 * EINTR means the system call was interrupted by a signal and can also
 * be safely ignored here.
 *
 * @param index The index of the client socket inside _fds.
 * @return true if the client was removed, false otherwise.
 */
bool	Server::handleClientEvent(size_t index) {
	char	buffer[513];
	int		bytesRead;
	int		fd;
	size_t	clientIndex;
	
	if (index == 0 || index >= _fds.size())
		return false;
	clientIndex = index - 1;
	if (clientIndex >= _clients.size())
		throw std::runtime_error("Error: client index out of range.");
	fd = _fds[index].fd;
	if (_fds[index].revents & (POLLHUP | POLLERR | POLLNVAL)) {
		removeClient(index, "Connection closed");
		return true;
	}
	if (_fds[index].revents & POLLOUT)
		handleSendBuffer(clientIndex, "");
	if (_clients[clientIndex].shouldDisconnect() == true) {
		removeClient(index, "Connection closed");
		return true;
	}
	if ((_fds[index].revents & POLLIN) == 0)
		return false;
	bytesRead = recv(fd, buffer, 512, 0);
	if (bytesRead == 0) {
		std::cout << "[INFO] client fd " << fd << " disconnected." << std::endl;
		removeClient(index, "client disconnected");
		return true;
	}
	if (bytesRead == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		throw std::runtime_error(std::string("Error: recv failed: ") + std::strerror(errno));
	}
	buffer[bytesRead] = '\0';
	handleClientBuffer(clientIndex, std::string(buffer, bytesRead));
	return false;
}
