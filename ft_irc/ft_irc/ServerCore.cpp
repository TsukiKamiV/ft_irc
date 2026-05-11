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
 Server::run()
 └── Server::handlePollEvents()
 ├── 监听 fd 有事件
 │   └── Server::acceptNewClient()
 │
 └── client fd 有事件
 └── Server::handleClientEvent(index)
 ├── 检查 HUP / ERR / NVAL
 ├── recv(...)
 ├── bytesRead == 0 -> removeClient()
 ├── bytesRead == -1 -> 错误处理
 └── Server::handleClientBuffer(clientIndex, chunk)
 ├── Client::appendBuffer(chunk)
 ├── 在 Client::_buffer 中找 '\n'
 ├── 切出完整 line
 ├── 去掉行尾 '\r'
 └── 暂时打印 line
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
 * @brief 创建并初始化服务器监听 socket。
 *
 * 该函数负责完成以下步骤：
 * 1. 创建 IPv4 / TCP 监听 socket；
 * 2. 设置地址复用选项 `SO_REUSEADDR`；
 * 3. 将 socket 设为 non-blocking；
 * 4. 绑定到指定端口；
 * 5. 开始监听传入连接。
 *
 * @param port 服务器监听端口，取值应在 1 ~ 65535 之间。
 * @return 成功时返回监听 socket 的 fd；失败时返回 -1。
 */


int Server::createListeningSocket(long port) {
	int	serverFd;
	//opt -- option，一个用来设置选项值的变量
	int	opt;
	struct sockaddr_in addr;
	//这个结构体内部大概是这样：
	//struct sockaddr_in {
	//	short sin_family;
	//	unsigned short sin_port;
	//	struct in_addr sin_addr;
	//	char sin_zero[8];  // padding
	//};
	
	//创建监听socket, fd是这个socket的fd
	/**
	 * @param domain, type, protocol
	 *AF_INET（使用什么地址族） ： IPV4
	 *	创建一个基于IPv4的socket
	 *SOCK_STREAM：面向连接的字节流通信，通常对应的协议是TCP协议
	 *0: 协议号 - 让操作系统根据前两个参数自动选择默认协议
	 *整句意思：创建一个基于 IPv4、使用 TCP 字节流通信的 socket，并把它的 fd 返回给 serverFd
	 */
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1)
		return -1;
	/**
	 *@brief 给 serverFd 这个 socket 设置一个选项：开启 `SO_REUSEADDR` ——也就是允许地址快速复用
	 *function prototype: `setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))`
	 *@param 1. serverfd => 表示对哪个socket设置选项，改变配置
	 *		  2. SOL_	SOCKET => level参数，有SOL_SOCKET(socket通用层）、IPPROTO_TCP（TCP层）、IPPROTO_IP（IP层）
	 *		  3. SO_REUSEADDR => 选项名，表示“允许重用本地地址”。
	 */
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
	//listen => 10 => backlog (当服务器socket已经在监听的时候，内核为“等待被accept的新连接“准备的排队上限）或“等待接受的连接队列长度”
	if (listen(serverFd, MAX_CONNECTION) == -1) {
		close(serverFd);
		return -1;
	}
	return serverFd;
}

//先把 fd 当前的“配置”读出来 → 在原有配置上加一个 NONBLOCK → 再写回去
int	Server::setNonBlocking(int fd) {
	//int flags;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return 0;
	//flags = fcntl(fd, F_GETFL, 0);
	//if (flags == -1)
	//	return -1;
	//if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	//	return -1;
	//return 0;
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
 *Now the pollfd is clientfd + 1 (the pollfd[0] is the server listening socket
 *For the concern of potential complication, it might be useful to have a mapping function "findPollIndexByFd(clientFd)"
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
		throw std::runtime_error(std::string("Error: recv failed: ")
								 + std::strerror(errno));
	}
	buffer[bytesRead] = '\0';
	handleClientBuffer(clientIndex, std::string(buffer, bytesRead));
	return false;
}

