#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"

bool	Server::handleClientBuffer(size_t clientIndex, const std::string &chunk) {
	const int	fd = _clients[clientIndex].getFd();
	
	if (_clients[clientIndex].getBuffer().size() + chunk.size() > 4096) {
		std::cerr << "[ERROR] fd " << fd << " buffer overflow, disconnecting." << std::endl;
		const size_t	fdsIndex = clientIndex + 1;
		removeClient(fdsIndex, "Buffer overflow");
		return true;
	}
	_clients[clientIndex].appendBuffer(chunk);
	
	while (true) {
		clientIndex = 0;
		while (clientIndex < _clients.size() && _clients[clientIndex].getFd() != fd)
			++clientIndex;
		if (clientIndex >= _clients.size())
			return true;
		
		std::string	&buf = _clients[clientIndex].getBuffer();
		const size_t	pos = buf.find('\n');
		if (pos == std::string::npos)
			break;
		
		std::string	line = buf.substr(0, pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		
		buf.erase(0, pos + 1);
		
		std::cout << "[Buffer fd " << fd << "] " << line << std::endl;
		processLine(clientIndex, line);
	}
	return false;
}

void	Server::handleSendBuffer(size_t clientIndex, const std::string &chunk) {
	if (clientIndex >= _clients.size())
		return;
	
	const size_t	SEND_BUF_MAX = 131072;
	const int		fd = _clients[clientIndex].getFd();
	std::string		&sendBuffer = _clients[clientIndex].getSendBuffer();
	
	if (!chunk.empty()) {
		if (sendBuffer.size() > SEND_BUF_MAX) {
			std::cerr << "[WARN] fd " << fd
			<< " send buffer overflow (" << sendBuffer.size()
			<< " bytes), disconnecting." << std::endl;
			_clients[clientIndex].setShouldDisconnect(true);
			updateClientPollEvent(clientIndex);
			return;
		}
		_clients[clientIndex].appendSendBuffer(chunk);
	}
	while (!sendBuffer.empty()) {
		const ssize_t	sentBytes = send(fd, sendBuffer.c_str(), sendBuffer.size(), 0);
		if (sentBytes > 0) {
			sendBuffer.erase(0, static_cast<size_t>(sentBytes));
			continue;
		}
		if (sentBytes == 0) {
			_clients[clientIndex].setShouldDisconnect(true);
			break;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			break;
		_clients[clientIndex].setShouldDisconnect(true);
		break;
	}
	updateClientPollEvent(clientIndex);
}

void	Server::updateClientPollEvent(size_t clientIndex) {
	if (clientIndex >= _clients.size())
		return;
	const size_t	pollIndex = clientIndex + 1;
	if (pollIndex >= _fds.size())
		return;
	_fds[pollIndex].events = _clients[clientIndex].getSendBuffer().empty()
	? POLLIN
	: (POLLIN | POLLOUT);
}

void	Server::processLine(size_t clientIndex, const std::string &line) {
	if (line.empty())
		return;
	const parseMessage	msg = parseLine(line);
	if (msg.command.empty())
		return;
	
	if (msg.command == "PASS")
		handlePass(clientIndex, msg);
	else if (msg.command == "NICK")
		handleNick(clientIndex, msg);
	else if (msg.command == "USER")
		handleUser(clientIndex, msg);
	else if (msg.command == "JOIN")
		handleJoin(clientIndex, msg);
	else if (msg.command == "MODE")
		handleMode(clientIndex, msg);
	else if (msg.command == "KICK")
		handleKick(clientIndex, msg);
	else if (msg.command == "INVITE")
		handleInvite(clientIndex, msg);
	else if (msg.command == "TOPIC")
		handleTopic(clientIndex, msg);
	else if (msg.command == "PRIVMSG")
		handlePrivmsg(clientIndex, msg);
	else if (msg.command == "PING")
		handlePing(clientIndex, msg);
	else if (msg.command == "PONG")
		handlePong(clientIndex, msg);
	else if (msg.command == "QUIT")
		handleQuit(clientIndex, msg);
	else if (msg.command == "PART")
		handlePart(clientIndex, msg);
	else if (msg.command == "CAP")
		return;
	else if (msg.command == "WHO")
		return;
	else {
		std::cout << "[INFO] unknown command: " << msg.command << std::endl;
		sendToClient(clientIndex,
					 Replies::ERR_UNKNOWNCOMMAND("localhost", getReplyTarget(clientIndex), msg.command));
	}
}

parseMessage	Server::parseLine(const std::string &line) {
	parseMessage	msg;
	size_t	i = 0;
	
	while (i < line.size() && line[i] == ' ')
		++i;
	if (i < line.size() && line[i] == ':') {
		while (i < line.size() && line[i] != ' ')
			++i;
		while (i < line.size() && line[i] == ' ')
			++i;
	}
	const size_t	start = i;
	while (i < line.size() && line[i] != ' ')
		++i;
	if (start < i) {
		msg.command = line.substr(start, i - start);
		for (size_t j = 0; j < msg.command.size(); ++j)
			msg.command[j] = static_cast<char>(
											   std::toupper(static_cast<unsigned char>(msg.command[j])));
	}
	while (i < line.size()) {
		while (i < line.size() && line[i] == ' ')
			++i;
		if (i >= line.size())
			break;
		if (line[i] == ':') {
			msg.hasTrailingParam = true;
			msg.params.push_back(line.substr(i + 1));
			break;
		}
		const size_t	pStart = i;
		while (i < line.size() && line[i] != ' ')
			++i;
		msg.params.push_back(line.substr(pStart, i - pStart));
	}
	return msg;
}

void	Server::sendToClient(size_t clientIndex, const std::string &message) {
	if (clientIndex >= _clients.size() || message.empty())
		return;
	handleSendBuffer(clientIndex, message);
}

void	Server::channelBroadcast(int channelIndex, const std::string &message) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const std::vector<int>	&memberFds = _channels[channelIndex].getMemberFds();
	for (size_t i = 0; i < memberFds.size(); ++i) {
		for (size_t j = 0; j < _clients.size(); ++j) {
			if (_clients[j].getFd() == memberFds[i]) {
				sendToClient(j, message);
				break;
			}
		}
	}
}

void	Server::channelBroadcastExcept(int channelIndex, const std::string &message, int exceptFd) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const std::vector<int>	&memberFds = _channels[channelIndex].getMemberFds();
	for (size_t i = 0; i < memberFds.size(); ++i) {
		if (memberFds[i] == exceptFd)
			continue;
		for (size_t j = 0; j < _clients.size(); ++j) {
			if (_clients[j].getFd() == memberFds[i]) {
				sendToClient(j, message);
				break;
			}
		}
	}
}

void	Server::broadcastNickChange(int clientFd, const std::string &oldNick, const std::string &newNick) {
	size_t	sourceIndex = 0;
	while (sourceIndex < _clients.size() && _clients[sourceIndex].getFd() != clientFd)
		++sourceIndex;
	if (sourceIndex >= _clients.size())
		return;
	
	const std::string	oldPrefix = oldNick + "!"
	+ _clients[sourceIndex].getUsername() + "@"
	+ _clients[sourceIndex].getHostField();
	
	for (size_t ci = 0; ci < _channels.size(); ++ci) {
		if (!_channels[ci].hasMember(clientFd))
			continue;
		const std::vector<int>	&memberFds = _channels[ci].getMemberFds();
		for (size_t mi = 0; mi < memberFds.size(); ++mi) {
			if (memberFds[mi] == clientFd)
				continue;
			for (size_t ki = 0; ki < _clients.size(); ++ki) {
				if (_clients[ki].getFd() == memberFds[mi]) {
					sendToClient(ki, Replies::RPL_NICK(oldPrefix, newNick));
					break;
				}
			}
		}
	}
}

std::string	Server::getReplyTarget(size_t clientIndex) const {
	if (clientIndex >= _clients.size() || _clients[clientIndex].getNick().empty())
		return "*";
	return _clients[clientIndex].getNick();
}
