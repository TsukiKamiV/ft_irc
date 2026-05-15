#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"

void	Server::handleJoin(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return;
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[JOIN] client fd " << _clients[clientIndex].getFd()
		<< " not registered, join refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return;
	}
	if (msg.params.empty()) {
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "JOIN"));
		return;
	}
	const std::string channelName = msg.params[0];
	if (channelName.size() < 2) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (channelName[0] != '#') {
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	if (channelName.size() > 200) {
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return;
	}
	
	int	channelIndex = findChannelIndex(channelName);
	const int	clientFd = _clients[clientIndex].getFd();
	
	if (channelIndex != -1) {
		if (_channels[channelIndex].hasMember(clientFd)) {
			std::cout << "[JOIN] client already in " << channelName << std::endl;
			return;
		}
		if (_channels[channelIndex].isInviteOnly() && !_channels[channelIndex].hasInvite(clientFd)) {
			sendToClient(clientIndex, Replies::ERR_INVITEONLYCHAN("localhost", getReplyTarget(clientIndex), channelName));
			return;
		}
		if (_channels[channelIndex].isKeyNeeded()) {
			if (msg.params.size() < 2 || msg.params[1] != _channels[channelIndex].getKey()) {
				sendToClient(clientIndex, Replies::ERR_BADCHANNELKEY("localhost", getReplyTarget(clientIndex), channelName));
				return;
			}
		}
		if (_channels[channelIndex].isNumLimited() && _channels[channelIndex].getMemberFds().size() >= static_cast<size_t>(_channels[channelIndex].getMaxMemberNum())) {
			sendToClient(clientIndex, Replies::ERR_CHANNELISFULL("localhost", getReplyTarget(clientIndex), channelName));
			return;
		}
		_channels[channelIndex].addMember(clientFd);
		std::cout << "[JOIN] joined existing channel " << channelName << std::endl;
	}
	else {
		_channels.push_back(Channel(channelName));
		channelIndex = static_cast<int>(_channels.size() - 1);
		_channels[channelIndex].addMember(clientFd);
		_channels[channelIndex].addOperator(clientFd);
		std::cout << "[JOIN] channel " << channelName
		<< " created by fd " << clientFd << std::endl;
	}
	
	if (_channels[channelIndex].hasInvite(clientFd))
		_channels[channelIndex].removeInvite(clientFd);
	
	channelBroadcast(channelIndex,
					 Replies::RPL_JOIN(_clients[clientIndex].getPrefix(), channelName));
	
	if (!_channels[channelIndex].getTopic().empty())
		sendToClient(clientIndex,
					 Replies::RPL_TOPIC("localhost", getReplyTarget(clientIndex),
										channelName, _channels[channelIndex].getTopic()));
	
	const std::string	nickList = buildNameReplyList(channelIndex);
	sendToClient(clientIndex, Replies::RPL_NAMREPLY("localhost", getReplyTarget(clientIndex), channelName, nickList));
	sendToClient(clientIndex, Replies::RPL_ENDOFNAMES("localhost", getReplyTarget(clientIndex), channelName));
}

std::string	Server::buildNameReplyList(int channelIndex) const {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return "";
	std::string	nickList;
	const std::vector<int>	&memberFds = _channels[channelIndex].getMemberFds();
	for (size_t mi = 0; mi < memberFds.size(); ++mi) {
		for (size_t ci = 0; ci < _clients.size(); ++ci) {
			if (_clients[ci].getFd() == memberFds[mi]) {
				if (!nickList.empty())
					nickList += " ";
				if (_channels[channelIndex].hasOperator(memberFds[mi]))
					nickList += "@";
				nickList += _clients[ci].getNick();
				break;
			}
		}
	}
	return nickList;
}

int	Server::findChannelIndex(const std::string &channelName) const {
	for (size_t i = 0; i < _channels.size(); ++i)
		if (_channels[i].getName() == channelName)
			return static_cast<int>(i);
	return -1;
}

void	Server::removeChannel(int channelIndex) {
	if (channelIndex < 0 || channelIndex >= static_cast<int>(_channels.size()))
		return;
	_channels.erase(_channels.begin() + channelIndex);
}

void	Server::removeClientFromChannels(size_t clientIndex, int clientFd, const std::string &reason) {
	if (clientIndex >= _clients.size())
		return;
	const std::string	prefix = _clients[clientIndex].getPrefix();
	
	size_t	i = 0;
	while (i < _channels.size()) {
		if (_channels[i].hasOperator(clientFd))
			_channels[i].removeOperator(clientFd);
		if (_channels[i].hasInvite(clientFd))
			_channels[i].removeInvite(clientFd);
		if (_channels[i].hasMember(clientFd)) {
			_channels[i].removeMember(clientFd);
			channelBroadcast(static_cast<int>(i), Replies::RPL_QUIT(prefix, reason));
		}
		if (_channels[i].isEmpty()) {
			_channels.erase(_channels.begin() + i);
			continue;
		}
		++i;
	}
}
