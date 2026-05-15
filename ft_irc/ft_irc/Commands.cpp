#include "Server.hpp"
#include "Replies.hpp"

void	Server::handleKick(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 2) {
		std::cout << "[KICK] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "KICK"));
		return;
	}
	const std::string	channelName = msg.params[0];
	const std::string	targetNick = msg.params[1];
	std::string	reason = "";
	if (msg.params.size() >= 3)
		reason = msg.params[2];
	
	const int	clientFd = _clients[clientIndex].getFd();
	
	if (channelName.empty() || targetNick.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "KICK"));
		return;
	}
	if (channelName[0] != '#') {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int	targetIndex = findClientIndexByNick(targetNick);
	if (targetIndex < 0) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return;
	}
	const int	targetFd = _clients[targetIndex].getFd();
	if (!_channels[channelIndex].hasMember(targetFd)) {
		sendToClient(clientIndex, Replies::ERR_USERNOTINCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return;
	}
	if (targetFd == clientFd) {
		sendToClient(clientIndex,
					 ":localhost NOTICE " + getReplyTarget(clientIndex)
					 + " :You cannot kick yourself\r\n");
		return;
	}
	_channels[channelIndex].removeMember(targetFd);
	const std::string	kickMsg = Replies::RPL_KICK(_clients[clientIndex].getPrefix(), channelName, targetNick, reason);
	channelBroadcast(channelIndex, kickMsg);
	sendToClient(static_cast<size_t>(targetIndex), kickMsg);
	std::cout << "[KICK] " << targetNick << " kicked from " << channelName << std::endl;
	if (_channels[channelIndex].isEmpty())
		removeChannel(channelIndex);
}

void	Server::handleInvite(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() < 2) {
		std::cout << "[INVITE] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "INVITE"));
		return;
	}
	const std::string	targetNick = msg.params[0];
	const std::string	channelName = msg.params[1];
	const int	clientFd = _clients[clientIndex].getFd();
	
	if (targetNick.empty() || channelName.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "INVITE"));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (channelName[0] != '#') {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int	targetIndex = findClientIndexByNick(targetNick);
	if (targetIndex < 0) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return;
	}
	if (!_clients[targetIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), targetNick));
		return;
	}
	const int	targetFd = _clients[targetIndex].getFd();
	if (_channels[channelIndex].hasMember(targetFd)) {
		sendToClient(clientIndex, Replies::ERR_USERONCHANNEL("localhost", getReplyTarget(clientIndex), targetNick, channelName));
		return;
	}
	_channels[channelIndex].addInvite(targetFd);
	std::cout << "[INVITE] " << targetNick << " invited to " << channelName << std::endl;
	sendToClient(clientIndex, Replies::RPL_INVITING("localhost", getReplyTarget(clientIndex), targetNick, channelName));
	sendToClient(static_cast<size_t>(targetIndex), Replies::RPL_INVITE(_clients[clientIndex].getPrefix(), targetNick, channelName));
}

void	Server::handleTopic(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.empty()) {
		std::cout << "[TOPIC] not enough parameters" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "TOPIC"));
		return;
	}
	const std::string	channelName = msg.params[0];
	const int	clientFd = _clients[clientIndex].getFd();
	
	if (channelName.empty() || channelName[0] != '#') {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	const int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (!_channels[channelIndex].hasMember(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (msg.params.size() == 1) {
		if (_channels[channelIndex].getTopic().empty())
			sendToClient(clientIndex, Replies::RPL_NOTOPIC("localhost", getReplyTarget(clientIndex), channelName));
		else
			sendToClient(clientIndex, Replies::RPL_TOPIC("localhost", getReplyTarget(clientIndex), channelName, _channels[channelIndex].getTopic()));
		return;
	}
	if (_channels[channelIndex].isTopicRestricted() && !_channels[channelIndex].hasOperator(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_CHANOPRIVSNEEDED("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const std::string	newTopic = msg.params[1];
	if (newTopic.size() > 307) {
		sendToClient(clientIndex, ":localhost NOTICE " + getReplyTarget(clientIndex) + " :TOPIC string too long (max 307 characters)\r\n");
		return;
	}
	_channels[channelIndex].setTopic(newTopic);
	if (newTopic.empty())
		std::cout << "[TOPIC] topic cleared on " << channelName << std::endl;
	else
		std::cout << "[TOPIC] topic changed on " << channelName << " to: " << newTopic << std::endl;
	channelBroadcast(channelIndex, Replies::RPL_TOPICCMD(_clients[clientIndex].getPrefix(), channelName, newTopic));
}

void	Server::handlePrivmsg(size_t clientIndex, const parseMessage &msg) {
	const int	clientFd = _clients[clientIndex].getFd();
	
	if (msg.params.size() < 2) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PRIVMSG"));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	const std::string	messageTarget = msg.params[0];
	const std::string	messageBody = msg.params[1];
	
	if (messageTarget.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PRIVMSG"));
		return;
	}
	if (messageBody.empty()) {
		sendToClient(clientIndex, Replies::ERR_NOTEXTTOSEND("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (messageTarget[0] == '#') {
		const int	channelIndex = findChannelIndex(messageTarget);
		if (channelIndex == -1) {
			sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), messageTarget));
			return;
		}
		if (!_channels[channelIndex].hasMember(clientFd)) {
			sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), messageTarget));
			return;
		}
		channelBroadcastExcept(channelIndex, Replies::RPL_PRIVMSG(_clients[clientIndex].getPrefix(), messageTarget, messageBody), clientFd);
		return;
	}
	const int	targetIndex = findClientIndexByNick(messageTarget);
	if (targetIndex < 0) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), messageTarget));
		return;
	}
	if (!_clients[targetIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHNICK("localhost", getReplyTarget(clientIndex), messageTarget));
		return;
	}
	sendToClient(static_cast<size_t>(targetIndex), Replies::RPL_PRIVMSG(_clients[clientIndex].getPrefix(), messageTarget, messageBody));
}

void	Server::handlePing(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.empty() || msg.params[0].empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PING"));
		return;
	}
	sendToClient(clientIndex, Replies::RPL_PONG("localhost", msg.params[0]));
}

void	Server::handlePong(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.size() != 1 || msg.params[0].empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PONG"));
		return;
	}
	std::cout << "[PONG] <" << msg.params[0] << "> received from fd "
	<< _clients[clientIndex].getFd() << std::endl;
}

void	Server::handleQuit(size_t clientIndex, const parseMessage &msg) {
	std::string	reason = "Client Quit";
	if (msg.params.size() >= 1 && !msg.params[0].empty())
		reason = msg.params[0];
	
	std::cout << "[QUIT] client fd " << _clients[clientIndex].getFd()
	<< " quit: " << reason << std::endl;
	
	const std::string	errorMsg = "ERROR :Closing Link: "
	+ _clients[clientIndex].getHostField()
	+ " (" + reason + ")\r\n";
	sendToClient(clientIndex, errorMsg);
	const size_t	fdsIndex = clientIndex + 1;
	removeClient(fdsIndex, reason);
}

void	Server::handlePart(size_t clientIndex, const parseMessage &msg) {
	if (msg.params.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PART"));
		return;
	}
	const std::string	channelName = msg.params[0];
	std::string	reason = "";
	if (msg.params.size() >= 2)
		reason = msg.params[1];
	
	if (channelName.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "PART"));
		return;
	}
	if (!_clients[clientIndex].isRegistered()) {
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (channelName[0] != '#') {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int	channelIndex = findChannelIndex(channelName);
	if (channelIndex == -1) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	const int	clientFd = _clients[clientIndex].getFd();
	if (!_channels[channelIndex].hasMember(clientFd)) {
		sendToClient(clientIndex, Replies::ERR_NOTONCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	channelBroadcast(channelIndex, Replies::RPL_PART(_clients[clientIndex].getPrefix(), channelName, reason));
	_channels[channelIndex].removeMember(clientFd);
	if (_channels[channelIndex].isEmpty())
		removeChannel(channelIndex);
}
