#include "Server.hpp"
#include "Replies.hpp"

void	Server::handlePass(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return;
	if (_clients[clientIndex].isPassApprouved()) {
		std::cout << "[PASS] client has already entered password" << std::endl;
		sendToClient(clientIndex, Replies::ERR_ALREADYREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (msg.params.size() != 1) {
		std::cout << "[PASS] invalid parameter count" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PASS"));
		return;
	}
	if (msg.params[0] != _password) {
		std::cout << "[PASS] wrong password" << std::endl;
		sendToClient(clientIndex, Replies::ERR_PASSWDMISMATCH("localhost", getReplyTarget(clientIndex)));
		return;
	}
	_clients[clientIndex].setPassApprouved(true);
	std::cout << "[PASS] password accepted for fd " << _clients[clientIndex].getFd() << std::endl;
	checkRegistration(clientIndex);
}

static bool	isValidNickFirstChar(char c) {
	return (std::isalpha(static_cast<unsigned char>(c))
			|| c == '[' || c == ']' || c == '\\' || c == '`'
			|| c == '^' || c == '{' || c == '}' || c == '|' || c == '_');
}

static bool	isValidNickChar(char c) {
	return (std::isalpha(static_cast<unsigned char>(c))
			|| std::isdigit(static_cast<unsigned char>(c))
			|| c == '[' || c == ']' || c == '\\' || c == '`'
			|| c == '^' || c == '{' || c == '}' || c == '|' || c == '_' || c == '-');
}

static bool	isValidNick(const std::string &nick) {
	if (nick.empty() || nick.size() > 30)
		return false;
	if (!isValidNickFirstChar(nick[0]))
		return false;
	for (size_t i = 1; i < nick.size(); ++i)
		if (!isValidNickChar(nick[i]))
			return false;
	return true;
}

void	Server::handleNick(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return;
	if (msg.params.empty() || msg.params[0].empty()) {
		std::cout << "[NICK] no nickname given" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NONICKNAMEGIVEN("localhost", getReplyTarget(clientIndex)));
		return;
	}
	const std::string	&newNick = msg.params[0];
	if (!isValidNick(newNick)) {
		std::cout << "[NICK] erroneous nickname: " << newNick << std::endl;
		sendToClient(clientIndex, Replies::ERR_ERRONEUSNICKNAME("localhost", getReplyTarget(clientIndex), newNick));
		return;
	}
	for (size_t i = 0; i < _clients.size(); ++i) {
		if (i != clientIndex && newNick == _clients[i].getNick()) {
			std::cout << "[NICK] nickname already in use" << std::endl;
			sendToClient(clientIndex, Replies::ERR_NICKNAMEINUSE("localhost", getReplyTarget(clientIndex), newNick));
			return;
		}
	}
	const std::string	oldNick = _clients[clientIndex].getNick();
	const int	clientFd = _clients[clientIndex].getFd();
	if (oldNick == newNick)
		return;
	_clients[clientIndex].setNick(newNick);
	std::cout << "[NICK] fd" << clientFd << " set nickname to " << newNick << std::endl;
	if (_clients[clientIndex].isRegistered() && !oldNick.empty()) {
		sendToClient(clientIndex, Replies::RPL_NICK(
													oldNick + "!" + _clients[clientIndex].getUsername() + "@" + _clients[clientIndex].getHostField(),
													newNick));
		broadcastNickChange(clientFd, oldNick, newNick);
		return;
	}
	checkRegistration(clientIndex);
}

void	Server::handleUser(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return;
	if (msg.params.size() < 4) {
		std::cout << "[USER] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "USER"));
		return;
	}
	for (size_t i = 0; i < 4; ++i) {
		if (msg.params[i].empty()) {
			std::cout << "[USER] empty parameter at position " << i << std::endl;
			sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "USER"));
			return;
		}
	}
	if (_clients[clientIndex].isRegistered()) {
		std::cout << "[USER] client already registered" << std::endl;
		sendToClient(clientIndex, Replies::ERR_ALREADYREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (!_clients[clientIndex].getUsername().empty()) {
		std::cout << "[USER] username already set" << std::endl;
		sendToClient(clientIndex, Replies::ERR_ALREADYREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	_clients[clientIndex].setUsername(msg.params);
	std::cout << "[USER] fd " << _clients[clientIndex].getFd()
	<< " set username to " << _clients[clientIndex].getUsername() << std::endl;
	checkRegistration(clientIndex);
}

void	Server::checkRegistration(size_t clientIndex) {
	if (clientIndex >= _clients.size())
		return;
	if (_clients[clientIndex].isPassApprouved()
		&& !_clients[clientIndex].getNick().empty()
		&& !_clients[clientIndex].getUsername().empty()) {
		_clients[clientIndex].setRegistered(true);
		std::cout << "[REGISTER] client fd " << _clients[clientIndex].getFd() << " registered." << std::endl;
		sendToClient(clientIndex, Replies::RPL_WELCOME("localhost", getReplyTarget(clientIndex)));
	}
}

bool	Server::isClientRegistered(size_t clientIndex) const {
	if (clientIndex >= _clients.size())
		return false;
	return _clients[clientIndex].isRegistered();
}

int	Server::findClientIndexByNick(const std::string &nick) const {
	for (size_t i = 0; i < _clients.size(); ++i)
		if (_clients[i].getNick() == nick)
			return static_cast<int>(i);
	return -1;
}
