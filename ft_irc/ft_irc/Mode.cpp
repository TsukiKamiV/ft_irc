#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include <cerrno>
#include <cstdlib>
#include <climits>
#include <sstream>

bool	Server::checkModeParams(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return false;
	
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[MODE] client not registered" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return false;
	}
	if (msg.params.empty()) {
		std::cout << "[MODE] missing target" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
		return false;
	}
	if (msg.params[0].empty()) {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), ""));
		return false;
	}
	if (msg.params[0][0] != '#') {
		std::cout << "[MODE] user mode ignored" << std::endl;
		return false;
	}
	if (msg.params.size() == 1) {
		handleChannelModeQuery(clientIndex, msg.params[0]);
		return false;
	}
	const int	channelIndex = findChannelIndex(msg.params[0]);
	if (channelIndex == -1) {
		std::cout << "[MODE] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), msg.params[0]));
		return false;
	}
	if (msg.params[1].empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
		return false;
	}
	const int	clientFd = _clients[clientIndex].getFd();
	if (!_channels[channelIndex].hasMember(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), msg.params[0]));
		return false;
	}
	return true;
}

void	Server::handleMode(size_t clientIndex, const parseMessage &msg) {
	if (!checkModeParams(clientIndex, msg))
		return;
	
	const std::string	channelName = msg.params[0];
	const std::string	modeString = msg.params[1];
	const int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1)
		return;
	
	char	sign = '+';
	size_t	paramIdx = 2;
	
	for (size_t i = 0; i < modeString.size(); ++i) {
		const char	c = modeString[i];
		
		if (c == '+' || c == '-') {
			sign = c;
			continue;
		}
		
		std::string	modeFull(1, sign);
		modeFull += c;
		
		if (c == 'o') {
			if (paramIdx >= msg.params.size()) {
				sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
				return;
			}
			const std::string	targetNick = msg.params[paramIdx++];
			const int	nickIndex = findClientIndexByNick(targetNick);
			if (nickIndex == -1) {
				sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
				continue;
			}
			const int	targetFd = _clients[nickIndex].getFd();
			operatorManager(clientIndex, modeFull, channelIndex, targetFd, targetNick);
		}
		else if (c == 'i') {
			inviteOnlyManager(clientIndex, modeFull, channelIndex);
		}
		else if (c == 't') {
			topicManager(clientIndex, modeFull, channelIndex);
		}
		else if (c == 'k') {
			std::string	key = "";
			if (sign == '+') {
				if (paramIdx >= msg.params.size()) {
					sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
					return;
				}
				key = msg.params[paramIdx++];
			} else {
				if (paramIdx < msg.params.size())
					++paramIdx;
			}
			keyManager(clientIndex, modeFull, key, channelIndex);
		}
		else if (c == 'l') {
			long	maxNumber = 0;
			if (sign == '+') {
				if (paramIdx >= msg.params.size()) {
					sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
					return;
				}
				const std::string	&limitStr = msg.params[paramIdx];
				char	*end = NULL;
				errno = 0;
				maxNumber = std::strtol(limitStr.c_str(), &end, 10);
				++paramIdx;
				if (errno == ERANGE || end == limitStr.c_str() || *end != '\0'
					|| maxNumber <= 0 || maxNumber > INT_MAX) {
					std::cout << "[MODE] invalid limit number" << std::endl;
					sendToClient(clientIndex,
								 ":localhost NOTICE " + getReplyTarget(clientIndex)
								 + " :invalid limit number\r\n");
					continue;
				}
			}
			maxMemberManager(clientIndex, modeFull, static_cast<int>(maxNumber), channelIndex);
		}
		else {
			std::cout << "[MODE] unknown mode char: " << c << std::endl;
			sendToClient(clientIndex,
						 Replies::ERR_UNKNOWNMODE("localhost", getReplyTarget(clientIndex), std::string(1, c)));
		}
	}
}

void	Server::handleChannelModeQuery(size_t clientIndex, const std::string &channelName) {
	const int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	std::string	modeString = "+";
	if (_channels[channelIndex].isInviteOnly())
		modeString += "i";
	if (_channels[channelIndex].isTopicRestricted())
		modeString += "t";
	if (_channels[channelIndex].isKeyNeeded())
		modeString += "k";
	if (_channels[channelIndex].isNumLimited())
		modeString += "l";
	sendToClient(clientIndex,
				 ":localhost 324 " + getReplyTarget(clientIndex) + " " + channelName + " " + modeString + "\r\n");
}

void	Server::operatorManager(size_t clientIndex, const std::string &modeString, int channelIndex, int targetFd, const std::string &targetNick) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const std::string	channelName = _channels[channelIndex].getName();
	
	if (!_channels[channelIndex].hasOperator(_clients[clientIndex].getFd())) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_channels[channelIndex].hasMember(targetFd)) {
		sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return;
	}
	if (modeString == "+o") {
		const int	res = _channels[channelIndex].addOperator(targetFd);
		if (res == Channel::OPERATOR_ADDED)
			channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+o", targetNick));
		else if (res == Channel::OPERATOR_NOT_MEMBER)
			sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
	}
	else if (modeString == "-o") {
		if (_channels[channelIndex].getOperatorFds().size() == 1 && _channels[channelIndex].hasOperator(targetFd)) {
			std::cout << "[MODE] cannot remove last operator" << std::endl;
			sendToClient(clientIndex,
						 ":localhost NOTICE " + getReplyTarget(clientIndex)
						 + " :Cannot remove the last operator from " + channelName + "\r\n");
			return;
		}
		const int	res = _channels[channelIndex].removeOperator(targetFd);
		if (res == Channel::OPERATOR_REMOVED)
			channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-o", targetNick));
	}
}

void	Server::inviteOnlyManager(size_t clientIndex, const std::string &modeString, int channelIndex) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const int	clientFd = _clients[clientIndex].getFd();
	const std::string	channelName = _channels[channelIndex].getName();
	
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (modeString == "+i") {
		if (_channels[channelIndex].isInviteOnly())
			return;
		_channels[channelIndex].setInviteOnly(true);
		std::cout << "[MODE] +i set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+i", ""));
	}
	else if (modeString == "-i") {
		if (!_channels[channelIndex].isInviteOnly())
			return;
		_channels[channelIndex].setInviteOnly(false);
		std::cout << "[MODE] -i set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-i", ""));
	}
}

void	Server::topicManager(size_t clientIndex, const std::string &modeString, int channelIndex) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const int	clientFd = _clients[clientIndex].getFd();
	const std::string	channelName = _channels[channelIndex].getName();
	
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (modeString == "+t") {
		if (_channels[channelIndex].isTopicRestricted())
			return;
		_channels[channelIndex].setTopicRestricted(true);
		std::cout << "[MODE] +t set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+t", ""));
	}
	else if (modeString == "-t") {
		if (!_channels[channelIndex].isTopicRestricted())
			return;
		_channels[channelIndex].setTopicRestricted(false);
		std::cout << "[MODE] -t set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-t", ""));
	}
}

void	Server::keyManager(size_t clientIndex, const std::string &modeString, const std::string &key, int channelIndex) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const int	clientFd = _clients[clientIndex].getFd();
	const std::string	channelName = _channels[channelIndex].getName();
	
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (modeString == "+k") {
		if (key.empty()) {
			sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
			return;
		}
		if (key.size() > 10) {
			sendToClient(clientIndex,
						 ":localhost NOTICE " + getReplyTarget(clientIndex)
						 + " :Channel key too long, max length: 10 characters\r\n");
			return;
		}
		_channels[channelIndex].setKey(key);
		std::cout << "[MODE] +k set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+k", key));
	}
	else if (modeString == "-k") {
		if (!_channels[channelIndex].isKeyNeeded())
			return;
		_channels[channelIndex].removeKey();
		std::cout << "[MODE] -k set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-k", ""));
	}
}

void	Server::maxMemberManager(size_t clientIndex, const std::string &modeString, int maxNumber, int channelIndex) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	const int	clientFd = _clients[clientIndex].getFd();
	const std::string	channelName = _channels[channelIndex].getName();
	
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (modeString == "+l") {
		_channels[channelIndex].setMaxMemberNum(maxNumber);
		std::ostringstream oss;
		oss << maxNumber;
		std::cout << "[MODE] +l " << maxNumber << " set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+l", oss.str()));
	}
	else if (modeString == "-l") {
		if (!_channels[channelIndex].isNumLimited())
			return;
		_channels[channelIndex].removeMaxMemberNum();
		std::cout << "[MODE] -l set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-l", ""));
	}
}
