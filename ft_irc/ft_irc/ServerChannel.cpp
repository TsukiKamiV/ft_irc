#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

void	Server::handleJoin(size_t clientIndex, const parseMessage &msg) {
	if (clientIndex >= _clients.size())
		return ;
	if (!_clients[clientIndex].isRegistered()) {
		std::cout << "[JOIN] client fd " << _clients[clientIndex].getFd() << " is not registered, join refused" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOTREGISTERED("localhost", getReplyTarget(clientIndex)));
		return ;
	}
	if (msg.params.size() < 1) {
		std::cout << "[JOIN] join command params not enough" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NEEDMOREPARAMS("localhost", getReplyTarget(clientIndex), "JOIN"));
		return ;
	}
	//if (msg.params.size() > 2) {
	//	std::cout << "[JOIN] invalid join format" << std::endl;
	//	sendToClient(clientIndex, "[JOIN] invalid format\r\n");
	//	return ;
	//}
	std::string channelName = msg.params[0];
	if (channelName.empty() || channelName.size() < 2) {
		std::cout << "[JOIN] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (channelName[0] != '#') {
		std::cout << "[JOIN] invalid channel name" << std::endl;
		sendToClient(clientIndex, Replies::ERR_BADCHANMASK("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	if (channelName.size() > 20) {
		std::cout << "[JOIN] channel name too long" << std::endl;
		sendToClient(clientIndex, Replies::ERR_NOSUCHCHANNEL("localhost", getReplyTarget(clientIndex), channelName));
		return ;
	}
	int channelIndex = findChannelIndex(channelName);
	int	clientFd = _clients[clientIndex].getFd();
	if (channelIndex != -1) {
		std::cout << "[JOIN] channel name found" << std::endl;
		if (_channels[channelIndex].hasMember(_clients[clientIndex].getFd())) {
			std::cout << "[JOIN] client already in channel" << std::endl;
			//sendToClient(clientIndex, "[JOIN] already in channel\r\n");
			return ;
		}
		if (_channels[channelIndex].isInviteOnly()) {
			if (!_channels[channelIndex].hasInvite(clientFd)) {
				std::cout << "[JOIN] client not invited to join the invite-only channel" << std::endl;
				sendToClient(clientIndex, Replies::ERR_INVITEONLYCHAN("localhost", getReplyTarget(clientIndex), channelName));
				return ;
			}
		}
		if (_channels[channelIndex].isKeyNeeded()) {
			if (msg.params.size() != 2) {
				std::cout << "[JOIN] channel key needed" << std::endl;
				sendToClient(clientIndex, Replies::ERR_BADCHANNELKEY("localhost", getReplyTarget(clientIndex), channelName));
				return ;
			}
			if (msg.params[1] != _channels[channelIndex].getKey()) {
				std::cout << "[JOIN] wrong channel key" << std::endl;
				sendToClient(clientIndex, Replies::ERR_BADCHANNELKEY("localhost", getReplyTarget(clientIndex), channelName));
				return ;
			}
		}
		if (_channels[channelIndex].isNumLimited()) {
			if (_channels[channelIndex].getMemberFds().size() >= static_cast<size_t>(_channels[channelIndex].getMaxMemberNum())) {
				std::cout << "[JOIN] channel member limit reached, join refused" << std::endl;
				sendToClient(clientIndex, Replies::ERR_CHANNELISFULL("localhost", getReplyTarget(clientIndex), channelName));
				return ;
			}
		}
		_channels[channelIndex].addMember(_clients[clientIndex].getFd());
		std::cout << "[JOIN] joined existing channel " << channelName << std::endl;
	}
	else {
		_channels.push_back(channelName);
		channelIndex = static_cast<int>(_channels.size() - 1);
		_channels[channelIndex].addMember(clientFd);
		std::cout << "[JOIN] channel " << channelName << " created by fd " << clientFd << std::endl;
		_channels[channelIndex].addOperator(clientFd);
	}
	std::string channelTopic = _channels[channelIndex].getTopic();
	if (channelTopic.empty())
		channelTopic = "no topic is set";
	std::string nickList = buildNameReplyList(channelIndex);
	sendToClient(clientIndex, Replies::RPL_NAMREPLY("localhost", getReplyTarget(clientIndex), channelName, nickList));
	sendToClient(clientIndex, Replies::RPL_ENDOFNAMES("localhost", getReplyTarget(clientIndex), channelName));
	if (_channels[channelIndex].hasInvite(clientFd))
		_channels[channelIndex].removeInvite(clientFd);
	std::string prefix = _clients[clientIndex].getPrefix();
	channelBroadcast(channelIndex, Replies::RPL_JOIN(_clients[clientIndex].getPrefix(), channelName));
}

std::string	Server::buildNameReplyList(int channelIndex) const
{
	std::string				nickList;
	const std::vector<int>	&memberFds = _channels[channelIndex].getMemberFds();
	size_t					memberIndex;
	size_t					clientIndex;
	int						memberFd;
	
	memberIndex = 0;
	while (memberIndex < memberFds.size()) {
		memberFd = memberFds[memberIndex];
		clientIndex = 0;
		while (clientIndex < _clients.size()) {
			if (_clients[clientIndex].getFd() == memberFd) {
				if (!nickList.empty())
					nickList += " ";
				if (_channels[channelIndex].hasOperator(memberFd))
					nickList += "@";
				nickList += _clients[clientIndex].getNick();
				break ;
			}
			clientIndex++;
		}
		memberIndex++;
	}
	return (nickList);
}

int	Server::findChannelIndex(const std::string &channelName) const {
	for (size_t i = 0; i < _channels.size(); ++i) {
		if (_channels[i].getName() == channelName)
			return (static_cast<int> (i));
	}
	return -1;
}

void	Server::removeChannel(int channelIndex) {
	if (channelIndex < 0)
		return ;
	if (channelIndex >= static_cast<int>(_channels.size()))
		return ;
	_channels.erase(_channels.begin() + channelIndex);
}

void	Server::removeClientFromChannels(size_t index, int clientFd, const std::string &reason) {
	size_t	i = 0;
	std::string nick = _clients[index].getNick();
	std::string prefix = _clients[index].getPrefix();
	
	while (i < _channels.size()) {
		if (_channels[i].hasOperator(clientFd))
			_channels[i].removeOperator(clientFd);
		if (_channels[i].hasInvite(clientFd))
			_channels[i].removeInvite(clientFd);
		if (_channels[i].hasMember(clientFd)) {
			_channels[i].removeMember(clientFd);
			channelBroadcast(static_cast<int> (i), Replies::RPL_QUIT(prefix, reason));
		}
		if (_channels[i].isEmpty()) {
			_channels.erase(_channels.begin() + i);
			continue;
		}
		i++;
	}
}
