#include "Server.hpp"


void	Server::handleKick(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 2){
		std::cout << "[KICK] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "KICK"));
		return ;
	}
		
	std::string channelName = msg.params[0];
	std::string targetNick = msg.params[1];
	std::string reason = "";
	if (msg.params.size() >= 3)
		reason = msg.params[2];
	int clientFd = _clients[clientIndex].getFd();
	
	if (channelName.empty() || targetNick.empty()) {
		std::cout << "[KICK] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "KICK"));
		return ;
	}
	if (channelName[0] != '#') {
		std::cout << "[KICK] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		std::cout << "[KICK] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[KICK] client fd " << _clients[clientIndex].getFd() << " is not registered, kick refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		std::cout << "[KICK] caller is not a member of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[KICK] caller is not an operator of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int	targetIndex = findClientIndexByNick(targetNick);
	int targetFd;
	if (targetIndex >= 0)
		targetFd = _clients[targetIndex].getFd();
	else {
		std::cout << "[KICK] target nick not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return ;
	}
	if (!_channels[channelIndex].hasMember(targetFd)) {
		std::cout << "[KICK] target is not a member of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return ;
	}
	//if (msg.params.size() < 2) {
	//	std::cout << "[KICK] invalid format" << std::endl;
	//	sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", //getReplyTarget(clientIndex), "KICK"));
	//	return ;
	//}
	if (msg.params.size() == 3 && msg.hasTrailingParam == false) {
		std::cout << "[KICK] invalid reason format" << std::endl;
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Invalid KICK format, use: KICK #channel nickname :reason\r\n");
		return ;
	}
	//if (msg.hasTrailingParam == true && msg.params[2].empty()) {
	//	std::cout << "[KICK] empty reason" << std::endl;
	//	sendToClient(clientIndex, "[KICK] reason cannot be empty\r\n");
	//	return ;
	//}
	//if (_channels[channelIndex].hasOperator(targetFd))
	//	_channels[channelIndex].removeOperator(targetFd);
	_channels[channelIndex].removeMember(targetFd);
	std::string prefix = _clients[targetIndex].getPrefix();
	std::cout << "[KICK] target kicked out from channel" << std::endl;
	//sendToClient(clientIndex, Replies::RPL_KICK(_clients[clientIndex].getPrefix(), channelName, targetNick, reason));
	channelBroadcast(channelIndex, Replies::RPL_KICK(_clients[clientIndex].getPrefix(), channelName, targetNick, reason));
	sendToClient(targetIndex, Replies::RPL_KICK(_clients[clientIndex].getPrefix(), channelName, targetNick, reason));
	//if (!reason.empty()) {
	//	sendToClient(targetIndex, "[KICK] reason: " + reason + "\r\n");
	//	channelBroadcast(channelIndex, "[KICK] " + channelName + " " + targetNick + " //(" + prefix + ") " + " kicked out from channel: " + reason + "\r\n");
	//}
	//else
	//	channelBroadcast(channelIndex, "[KICK] " + channelName + " " + targetNick + " //(" + prefix + ")" + " kicked out from channel\r\n");
	if (_channels[channelIndex].isEmpty())
		removeChannel(channelIndex);
}

void	Server::handleInvite(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 2) {
		std::cout << "[INVITE] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "INVITE"));
		return ;
	}
	std::string targetNick = msg.params[0];
	std::string channelName = msg.params[1];
	int clientFd = _clients[clientIndex].getFd();
	if (targetNick.empty() || channelName.empty()) {
		std::cout << "[INVITE] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "INVITE"));
		return ;
	}
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[INVITE] client fd " << clientFd << " is not registered, invite refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	if (channelName[0] != '#') {
		std::cout << "[INVITE] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		std::cout << "[INVITE] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		std::cout << "[INVITE] caller is not a member of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[INVITE] caller is not an operator of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int targetIndex = findClientIndexByNick(targetNick);
	int targetFd;
	if (targetIndex >= 0)
		targetFd = _clients[targetIndex].getFd();
	else {
		std::cout << "[INVITE] target nick not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return ;
	}
	if (!_clients[targetIndex].isRegistered()) {
		std::cout << "[INVITE] target is not registered" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return ;
	}
	if (_channels[channelIndex].hasMember(targetFd)) {
		std::cout << "[INVITE] target already on channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_USERONCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return ;
	}
	_channels[channelIndex].addInvite(targetFd);
	std::cout << "[INVITE] " << targetNick << " invited to " << channelName << std::endl;
	sendToClient(clientIndex, Replies::RPL_INVITING("localhost", getReplyTarget(clientIndex), targetNick, channelName));
	sendToClient(targetIndex, Replies::RPL_INVITE(_clients[clientIndex].getPrefix(), targetNick, channelName));
	//channelBroadcast(channelIndex, _clients[clientIndex].getNick() + " has invited " + targetNick + " to join the channel\r\n");
}


void	Server::   handleTopic(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 1) {
		std::cout << "[TOPIC] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "TOPIC"));
		return ;
	}
	std::string channelName = msg.params[0];
	int clientFd = _clients[clientIndex].getFd();
	if (channelName.empty() || channelName[0] != '#') {
		std::cout << "[TOPIC] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[TOPIC] client fd " << clientFd << " is not registered, topic refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	
	int channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		std::cout << "[TOPIC] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		std::cout << "[TOPIC] caller is not a member of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	
	if (msg.params.size() == 1) {
		if (_channels[channelIndex].getTopic().empty())
			sendToClient(clientIndex, Replies::RPL_NOTOPIC("localhost", getReplyTarget(clientIndex), channelName));
		else
			sendToClient(clientIndex, Replies::RPL_TOPIC("localhost", getReplyTarget(clientIndex), channelName, _channels[channelIndex].getTopic()));
		return ;
	}
	if (_channels[channelIndex].isTopicRestricted() && !_channels[channelIndex].hasOperator(clientFd)) {
		std::cout << "[TOPIC] caller is not channel operator" << std::endl;
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (msg.params.size() != 2 || msg.hasTrailingParam == false) {
		std::cout << "[TOPIC] invalid topic format" << std::endl;
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Invalid TOPIC format, use: TOPIC #channel :new topic\r\n");
		return ;
	}
	std::string newTopic = msg.params[1];
	if (newTopic.size() > 20) {
		std::cout << "[TOPIC] topic too long" << std::endl;
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :TOPIC string too long. Max length 20\r\n");
		return ;
	}
	if (newTopic.empty()) {
		if (_channels[channelIndex].getTopic().empty()) {
			sendToClient(clientIndex, Replies::RPL_NOTOPIC("localhost", getReplyTarget(clientIndex), channelName));
			return ;
		}
		_channels[channelIndex].setTopic("");
		std::cout << "[TOPIC] topic cleared on " << _channels[channelIndex].getName() << std::endl;
		//sendToClient(clientIndex, Replies::RPL_TOPICCMD(_clients[clientIndex].getPrefix(), channelName, ""));
		channelBroadcast(channelIndex, Replies::RPL_TOPICCMD(_clients[clientIndex].getPrefix(), channelName, ""));
	}
	else {
		_channels[channelIndex].setTopic(newTopic);
		std::cout << "[TOPIC] topic changed on " << _channels[channelIndex].getName() << std::endl;
		//sendToClient(clientIndex, "[TOPIC] topic changed\r\n");
		channelBroadcast(channelIndex, Replies::RPL_TOPICCMD(_clients[clientIndex].getPrefix(), channelName, newTopic));
	}
}

void	Server::handlePrivmsg(size_t clientIndex, const parseMessage &msg) {
	std::string	messageTarget;
	std::string	messageBody;
	std::string	formattedMessage;
	int			clientFd = _clients[clientIndex].getFd();
	int			targetIndex;
	int			channelIndex;
	int 		targetFd;
	size_t		i, j;
	const std::vector<int> *memberFds;
	
	if (msg.params.size() < 2) {
		std::cout << "[PRIVMSG] invalid format" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PRIVMSG"));
		return ;
	}
	if (msg.params.size() > 2 || msg.hasTrailingParam == false) {
		std::cout << "[PRIVMSG] invalid format" << std::endl;
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :Invalid PRIVMSG format\r\n");
		return ;
	}
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[PRIVMSG] client fd " << clientFd << " is not registered, PRIVMSG refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	messageTarget = msg.params[0];
	messageBody = msg.params[1];
	if (messageTarget.empty()) {
		std::cout << "[PRIVMSG] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PRIVMSG"));
		return ;
	}
	if (messageBody.empty()) {
		std::cout << "[PRIVMSG] no text to send" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTEXTTOSEND("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	if (messageTarget[0] == '#') {
		channelIndex = findChannelIndex(messageTarget);
		if (channelIndex == -1) {
			std::cout << "[PRIVMSG] channel not found" << std::endl;
			sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), messageTarget));
			return ;
		}
		if (!_channels[channelIndex].hasMember(clientFd)) {
			std::cout << "[PRIVMSG] caller is not a member of the channel" << std::endl;
			sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), messageTarget));
			return ;
		}
		memberFds = &_channels[channelIndex].getMemberFds();
		i = 0;
		while (i < memberFds->size()) {
			if ((*memberFds)[i] != clientFd) {
				j = 0;
				while (j < _clients.size()) {
					if (_clients[j].getFd() == (*memberFds)[i]) {
						sendToClient(j, Replies::RPL_PRIVMSG(_clients[clientIndex].getPrefix(), messageTarget, messageBody));
						break;
					}
					j++;
				}
			}
			i++;
		}
		return ;
	}
	targetIndex = findClientIndexByNick(messageTarget);
	if (targetIndex < 0) {
		std::cout << "[PRIVMSG] target nickname not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), messageTarget));
		return ;
	}
	if (!_clients[targetIndex].isRegistered()) {
		std::cout << "[PRIVMSG] target is not registered" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), messageTarget));
		return ;
	}
	targetFd = _clients[targetIndex].getFd();
	formattedMessage = _clients[clientIndex].getNick() + " PRIVMSG " + messageTarget + " :" + messageBody + "\r\n";
	if (targetFd == clientFd) {
		sendToClient(clientIndex, Replies::RPL_PRIVMSG(_clients[clientIndex].getPrefix(),messageTarget, messageBody));
		return ;
	}
	sendToClient(static_cast<size_t>(targetIndex), Replies::RPL_PRIVMSG(_clients[clientIndex].getPrefix(), messageTarget, messageBody));
}

void	Server::handlePing(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 1) {
		std::cout << "[PING] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PING"));
		return ;
	}
	if (msg.params[0].empty()) {
		std::cout << "[PING] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PING"));
		return ;
	}
	std::string token = msg.params[0];
	sendToClient(clientIndex, Replies::RPL_PONG("localhost", token));
}

void	Server::handlePong(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() != 1) {
		std::cout << "[PONG] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PONG"));
		return ;
	}
	if (msg.params[0].empty()) {
		std::cout << "[PONG] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PONG"));
		return ;
	}
	std::string token = msg.params[0];
	std::cout << "[PONG] <" + token + "> received from " << _clients[clientIndex].getFd() << std::endl;
	//sendToClient(clientIndex, "[PONG] token <" + token + "> received from " + _clients[clientIndex].getNick() +  "\r\n");
}

void	Server::handleQuit(size_t clientIndex, 	const parseMessage &msg) {
	std::string reason;
	size_t		fdsIndex;
	std::string	quitMsg;
	
	reason = "Client Quit";
	if (msg.params.size() >= 1 && msg.params[0].empty() == false)
		reason = msg.params[0];
	quitMsg = Replies::RPL_QUIT(_clients[clientIndex].getPrefix(), reason);
	std::cout << "[QUIT] client fd " << _clients[clientIndex].getFd() << " quit: " << reason << std::endl;
	fdsIndex = clientIndex + 1;
	removeClient(fdsIndex, reason);
}

void	Server::handlePart(size_t clientIndex, const parseMessage &msg) {
	std::string channelName;
	std::string reason = "";
	if (msg.params.size() < 1) {
		std::cout << "[PART] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PART"));
		return ;
	}
	channelName = msg.params[0];
	if (msg.params.size() == 2) {
		if (msg.hasTrailingParam == false) {
			std::cout << "[PART] invalid reason format" << std::endl;
			sendToClient(clientIndex, ":localhost NOTICE " + 	getReplyTarget(clientIndex) + " :Invalid PART format, use: PART 	#channel :reason\r\n");
			return ;
		}
		reason = msg.params[1];
	}
	if (channelName.empty()) {
		std::cout << "[PART] invalid parameter" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PART"));
		return ;
	}
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[PART] client fd " << _clients[clientIndex].getFd() << " is not registered, part refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	if (channelName[0] != '#') {
		std::cout << "[PART] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		std::cout << "[PART] channel not found" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	int clientFd = _clients[clientIndex].getFd();
	if (!_channels[channelIndex].hasMember(clientFd)) {
		std::cout << "[PART] caller is not a member of the channel" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	//int	channelIndex = findChannelIndex(channelName);
	channelBroadcast(channelIndex, Replies::RPL_PART(_clients[clientIndex].getPrefix(), channelName, reason));
	_channels[channelIndex].removeMember(clientFd);
	if (_channels[channelIndex].isEmpty())
		removeChannel(channelIndex);
}
