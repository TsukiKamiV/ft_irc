#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

/**
 * @brief Server I/O helpers for buffered IRC input and non-blocking output.
 *
 * This file contains the functions that connect poll()-based socket events
 * with the IRC command layer.
 *
 * Input side:
 * - handleClientBuffer() appends each recv() chunk to the client's private
 *   input buffer.
 * - Since TCP is a byte stream, one recv() call may contain a partial IRC
 *   command, one full command, or several commands at once.
 * - The buffer is therefore split only when a complete IRC line ending with
 *   '\n' is found.
 * - A trailing '\r' is removed before the line is parsed and dispatched.
 *
 * Output side:
 * - sendToClient() does not assume that send() can write the whole message
 *   immediately.
 * - handleSendBuffer() appends outgoing data to the client's send buffer and
 *   tries to flush it with send() on the non-blocking socket.
 * - If only part of the data is sent, the remaining bytes stay in the buffer.
 * - updateClientPollEvent() enables POLLOUT only while there is pending data
 *   to send, otherwise the socket only listens for POLLIN.
 *
 * Broadcast helpers reuse sendToClient(), so channel messages, nick changes,
 * and server replies all follow the same buffered non-blocking send path.
 */

bool	Server::handleClientBuffer(size_t clientIndex, const std::string &chunk) {
	int	fd = _clients[clientIndex].getFd();
	if (_clients[clientIndex].getBuffer().size() + chunk.size() > 4096) {
		std::cerr << "[ERROR] fd " << fd << " buffer overflow, disconnecting." << std::endl;
		size_t	fdsIndex = clientIndex + 1;
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
		size_t		pos = buf.find('\n');
		if (pos == std::string::npos)
			break;
		std::string line = buf.substr(0, pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		buf.erase(0, pos + 1);
		std::cout << "[Buffer fd " << fd << "] " << line << std::endl;
		processLine(clientIndex, line);
	}
	return false;
}

void	Server::handleSendBuffer(size_t index, const std::string &chunk) {
	if (index >= _clients.size())
		return ;
	std::string	&sendBuffer = _clients[index].getSendBuffer();
	ssize_t		sentBytes;
	int			fd;
	
	fd = _clients[index].getFd();
	if (!chunk.empty())
		_clients[index].appendSendBuffer(chunk);
	while (!sendBuffer.empty()) {
		sentBytes = send(fd, sendBuffer.c_str(), sendBuffer.size(), 0);
		if (sentBytes > 0) {
			sendBuffer.erase(0, sentBytes);
			continue;
		}
		if (sentBytes == 0) {
			_clients[index].setShouldDisconnect(true);
			break;
		}
		
		//throw std::runtime_error("Error: send returned 0");
		if (sentBytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				break ;
			_clients[index].setShouldDisconnect(true);
			break;
		}
	}
	updateClientPollEvent(index);
}

void	Server::updateClientPollEvent(size_t clientIndex) {
	size_t	pollIndex;
	if (clientIndex >= _clients.size())
		return ;
	pollIndex = clientIndex + 1;
	if (pollIndex >= _fds.size())
		return ;
	if (_clients[clientIndex].getSendBuffer().empty())
		_fds[pollIndex].events = POLLIN;
	else
		_fds[pollIndex].events = POLLIN | POLLOUT;
}

void	Server::processLine(size_t clientIndex, const std::string &line) {
	parseMessage msg;
	if (line.empty())
		return ;
	msg = parseLine(line);
	if (msg.command.empty())
		return ;
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
	else if (msg.command == "CAP")
		return ;
	else if (msg.command == "WHO")
		return ;
	else if (msg.command == "QUIT")
		handleQuit(clientIndex, msg);
	else if (msg.command == "PART")
		handlePart(clientIndex, msg);
	else {
		std::cout << "[INFO] Unknown command: " << msg.command << std::endl;
		sendToClient(clientIndex, Replies::ERR_UNKNOWNCOMMAND("localhost", getReplyTarget(clientIndex), msg.command));
	}
}

parseMessage	Server::parseLine(const std::string &line) {
	parseMessage 	msg;
	size_t			i;
	size_t			start;
	
	i = 0;
	while (i < line.size() && line[i] == ' ')
		i++;
	start = i;
	while (i < line.size() && line[i] != ' ')
		i++;
	if (start < i)
		msg.command = line.substr(start, i - start);
	while (i < line.size()) {
		while (i < line.size() && line[i] == ' ')
			i++;
		if (i >= line.size())
			break;
		if (line[i] == ':') {
			msg.hasTrailingParam = true;
			msg.params.push_back(line.substr(i + 1));
			break;
		}
		start = i;
		while (i < line.size() && line[i] != ' ')
			i++;
		msg.params.push_back(line.substr(start, i - start));
	}
	return msg;
}

void	Server::sendToClient(size_t clientIndex, const std::string &message) {
	if (clientIndex >= _clients.size())
		return ;
	if (message.empty())
		return ;
	handleSendBuffer(clientIndex, message);
	//updateClientPollEvent(clientIndex);
	//POLLIN => this fd is readable (has something inside to recv())
	//POLLOUT => this fd is writable (socket can continue to send())
	//when you have something left in _sendBuffer, you wait for poll to tell you "now you can continue to send
	
}

void	Server::channelBroadcast(int channelIndex, const std::string &message) {
	size_t	i = 0, j;
	if (channelIndex < 0)
		return ;
	if (channelIndex >= static_cast<int>(_channels.size()))
		return ;
	const std::vector<int> &memberFds = _channels[channelIndex].getMemberFds();
	while (i < memberFds.size()) {
		j = 0;
		while (j < _clients.size()) {
			if (_clients[j].getFd() == memberFds[i]) {
				sendToClient(j, message);
				break;
			}
			j++;
		}
		i++;
	}
}

void	Server::channelBroadcastExcept(int channelIndex, const std::string &message, int exceptFd) {
	size_t	i, j;
	const std::vector<int>	*memberFds;
	
	if (channelIndex < 0)
		return ;
	if (channelIndex >= static_cast<int>(_channels.size()))
		return ;
	memberFds = &_channels[channelIndex].getMemberFds();
	i = 0;
	while (i < memberFds->size()) {
		if ((*memberFds)[i] != exceptFd) {
			j = 0;
			while (j < _clients.size()) {
				if (_clients[j].getFd() == (*memberFds)[i]){
					sendToClient(j, message);
					break;
				}
				j++;
			}
		}
		i++;
	}
}

void	Server::broadcastNickChange(int clientFd, const std::string &oldNick, const std::string &newNick) {
	size_t	channelIndex;
	size_t	memberIndex;
	size_t	clientIndex;
	const std::vector<int>	*memberFds;
	std::string	oldPrefix;
	size_t		sourceIndex;
	
	sourceIndex = 0;
	while (sourceIndex < _clients.size()) {
		if (_clients[sourceIndex].getFd() == clientFd)
			break;
		sourceIndex++;
	}
	if (sourceIndex >= _clients.size())
		return ;
	oldPrefix = oldNick + "!" + _clients[sourceIndex].getUsername() + "@" + _clients[sourceIndex].getHostField();
	
	channelIndex = 0;
	while (channelIndex < _channels.size()) {
		if (_channels[channelIndex].hasMember(clientFd)) {
			memberFds = &_channels[channelIndex].getMemberFds();
			memberIndex = 0;
			while (memberIndex < memberFds->size()) {
				if ((*memberFds)[memberIndex] != clientFd) {
					clientIndex = 0;
					while (clientIndex < _clients.size()) {
						if (_clients[clientIndex].getFd() == (*memberFds)[memberIndex]) {
							sendToClient(clientIndex, Replies::RPL_NICK(oldPrefix, newNick));
							break;
						}
						clientIndex++;
					}
				}
				memberIndex++;
			}
		}
		channelIndex++;
	}
}

std::string	Server::getReplyTarget(size_t clientIndex) const {
	if (clientIndex >= _clients.size())
		return ("*");
	if (_clients[clientIndex].getNick().empty())
		return ("*");
	return (_clients[clientIndex].getNick());
}
