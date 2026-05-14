#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

//For original MODE command - edge case testing like "MODE #channel +k pass pass" in irssi:
//use `/quote MODE #channel +k pass pass`

bool	Server::checkModeParams(size_t clientIndex, const parseMessage &msg) {
	std::string modeString;
	int			channelIndex;
	
	if (clientIndex >= _clients.size())
		return false;
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[MODE] client not registered" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return false;
	}
	if (msg.params.size() < 1) {
		std::cout << "[MODE] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), msg.command));
		return false;
	}
	if (msg.params[0].empty()) {
		std::cout << "[MODE] empty target" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), msg.params[0]));
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
	if (msg.params.size() < 2) {
		std::cout << "[MODE] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), msg.command));
		return false;
	}
	channelIndex = findChannelIndex(msg.params[0]);
	if (channelIndex == -1) {
		std::cout << "[MODE] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), msg.params[0]));
		return false;
	}
	modeString = msg.params[1];
	if (modeString != "+o" && modeString != "-o"
		&& modeString != "+i" && modeString != "-i"
		&& modeString != "+t" && modeString != "-t"
		&& modeString != "+k" && modeString != "-k"
		&& modeString != "+l" && modeString != "-l") {
		std::cout << "[MODE] unsupported mode string" << std::endl;
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Unsupported mode string\r\n");
		return false;
	}
	if ((modeString == "+o" || modeString == "-o" || modeString == "+k" || modeString == "+l") && (msg.params.size() < 3 || msg.params[2].empty())) {
		std::cout << "[MODE] missing mode parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), msg.command));
		return false;
	}
	return true;
}

void	Server::handleMode(size_t clientIndex, const parseMessage &msg) {
	std::string channelName;
	std::string modeString;
	std::string targetNick;
	int			channelIndex;
	int			nickIndex;
	int			targetFd;
	
	if (!checkModeParams(clientIndex, msg))
		return ;
	channelName = msg.params[0];
	modeString = msg.params[1];
	channelIndex = findChannelIndex(channelName);
	if (msg.params.size() >= 3)
		targetNick = msg.params[2];
	if (modeString == "+o" || modeString == "-o") {
		nickIndex = findClientIndexByNick(targetNick);
		if (nickIndex == -1) {
			std::cout << "[MODE] target nickname not found" << std::endl;
			sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
			return ;
		}
		targetFd = _clients[nickIndex].getFd();
		operatorManager(clientIndex, modeString, channelIndex, targetFd, targetNick);
	}
	else if (modeString == "+i" || modeString == "-i") {
		inviteOnlyManager(clientIndex, modeString, channelIndex);
	}
	else if (modeString == "+t" || modeString == "-t") {
		topicManager(clientIndex, modeString, channelIndex);
	}
	else if (modeString == "+k" || modeString == "-k") {
		std::string key = "";
		if (modeString == "+k")
			key = msg.params[2];
		keyManager(clientIndex, modeString, key, channelIndex);
	}
	else if (modeString == "+l" || modeString == "-l") {
		long	maxNumber;
		char	*end;
		
		maxNumber = 0;
		if (modeString == "+l") {
			errno = 0;
			end = NULL;
			maxNumber = std::strtol(msg.params[2].c_str(), &end, 10);
			if (errno == ERANGE || end == msg.params[2].c_str() || *end != '\0'
				|| maxNumber <= 0 || maxNumber > INT_MAX) {
				std::cout << "[MODE] invalid limit number" << std::endl;
				sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :invalid limit number\r\n");
				return ;
			}
		}
		maxMemberManager(clientIndex, modeString, static_cast<int>(maxNumber), channelIndex);
	}
	else if (modeString == "b") {
		std::cout << "[MODE] ban list query ignored" << std::endl;
		return ;
	}
}

void	Server::handleChannelModeQuery(size_t clientIndex, const std::string &channelName) {
	int			channelIndex;
	std::string	modeString;
	std::string	paramString;
	
	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	modeString = "+";
	if (_channels[channelIndex].isInviteOnly())
		modeString += "i";
	if (_channels[channelIndex].isTopicRestricted())
		modeString += "t";
	if (_channels[channelIndex].isKeyNeeded())
		modeString += "k";
	if (_channels[channelIndex].isNumLimited())
		modeString += "l";
	sendToClient(clientIndex, ":localhost 324 " + getReplyTarget(clientIndex) + " " + channelName + " " + modeString + "\r\n");
}

void	Server::operatorManager(size_t clientIndex, const std::string &modeString, int channelIndex, int targetFd, const std::string &targetNick) {
	std::string channelName = _channels[channelIndex].getName();
	if (!_channels[channelIndex].hasOperator(_clients[clientIndex].getFd())) {
		std::cout << "[MODE] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_channels[channelIndex].hasMember(targetFd)) {
		sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return ;
	}
	if (modeString == "+o") {
		int res = _channels[channelIndex].addOperator(targetFd);
		std::string	modeMsg;
		
		if (res == Channel::OPERATOR_ADDED) {
			modeMsg = Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+o", targetNick);
			channelBroadcast(channelIndex, modeMsg);
		}
		else if (res == Channel::OPERATOR_NOT_MEMBER)
			sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		else if (res == Channel::OPERATOR_ALREADY_SET) {
			//sendToClient(clientIndex, "[MODE] target already operator\r\n");
			std::cout << "[MODE] target already channel operator" << std::endl;
			return ;
		}
	}
	else if (modeString == "-o") {
		if (_channels[channelIndex].getOperatorFds().size() == 1 && _channels[channelIndex].hasOperator(targetFd)) {
			std::cout << "[MODE] cannot remove the last operator of channel" << std::endl;
			sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Cannot remove the last operator from " + channelName + "\r\n");
			return ;
		}
		int res = _channels[channelIndex].removeOperator(targetFd);
		if (res == Channel::OPERATOR_NOT_FOUND)
			return ;
		else if (res == Channel::OPERATOR_REMOVED)
			channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-o", targetNick));
	}
}

void	Server::inviteOnlyManager(size_t clientIndex, const std::string &modeString, int channelIndex) {
	int clientFd = _clients[clientIndex].getFd();
	std::string channelName = _channels[channelIndex].getName();
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[MODE] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (modeString == "+i") {
		if (_channels[channelIndex].isInviteOnly()) {
			return ;
		}
		_channels[channelIndex].setInviteOnly(true);
		std::cout << "[MODE] invite-only enabled" << std::endl;
		channelBroadcast(channelIndex,
						 Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+i", ""));
	}
	else if (modeString == "-i") {
		if (!_channels[channelIndex].isInviteOnly()) {
			return ;
		}
		_channels[channelIndex].setInviteOnly(false);
		std::cout << "[MODE] invite-only disabled on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-i", ""));
	}
}

void	Server::topicManager(size_t clientIndex, const std::string &modeString, int channelIndex) {
	int clientFd = _clients[clientIndex].getFd();
	std::string	channelName = _channels[channelIndex].getName();
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[MODE] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (modeString == "+t") {
		if (_channels[channelIndex].isTopicRestricted()) {
			return ;
		}
		_channels[channelIndex].setTopicRestricted(true);
		std::cout << "[MODE] channel topic restricted" << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+t", ""));
	}
	else if (modeString == "-t") {
		if (!_channels[channelIndex].isTopicRestricted()) {
			return ;
		}
		_channels[channelIndex].setTopicRestricted(false);
		std::cout << "[MODE] channel topic restriction disabled on " << _channels[channelIndex].getName() << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-t", ""));
	}
}

void	Server::keyManager(size_t clientIndex, const std::string &modeString, const std::string &key, int channelIndex) {
	int	clientFd = _clients[clientIndex].getFd();
	std::string channelName = _channels[channelIndex].getName();
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[MODE] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (modeString == "+k") {
		if (key.empty()) {
			std::cout << "[MODE] missing channel key" << std::endl;
			sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "MODE"));
			return ;
		}
		if (key.size() > 10) {
			std::cout << "[MODE] channel key too long" << std::endl;
			sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Channel key too long, max length: 10 characters\r\n");
			return ;
		}
		_channels[channelIndex].setKey(key);
		std::cout << "[MODE] channel key set on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+k", key));
	}
	else if (modeString == "-k") {
		if (!_channels[channelIndex].isKeyNeeded()) {
			return ;
		}
		_channels[channelIndex].removeKey();
		std::cout << "[MODE] channel key disabled on " << channelName << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-k", ""));
	}
}

void	Server::maxMemberManager(size_t clientIndex, const std::string &modeString, int maxNumber, int channelIndex) {
	int clientFd = _clients[clientIndex].getFd();
	std::string channelName = _channels[channelIndex].getName();
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[MODE] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	
	if (modeString == "+l") {
		_channels[channelIndex].setMaxMemberNum(maxNumber);
		std::ostringstream oss;
		std::string limitString;
		
		oss << maxNumber;
		limitString = oss.str();
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "+l", limitString));
	}
	else if (modeString == "-l") {
		if (!_channels[channelIndex].isNumLimited()) {
			return ;
		}
		_channels[channelIndex].removeMaxMemberNum();
		std::cout << "[MODE] channel member limit disabled on " << _channels[channelIndex].getName() << std::endl;
		channelBroadcast(channelIndex, Replies::RPL_MODE(_clients[clientIndex].getPrefix(), channelName, "-l", ""));
	}
}
