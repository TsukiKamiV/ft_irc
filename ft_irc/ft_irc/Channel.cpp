#include "Channel.hpp"

Channel::Channel(const std::string &name): _name(name)
{
	_inviteOnly = false;
	_topicRestricted = false;
	_needKey = false;
	_numLimited = false;
	_maxMemberNum = INT_MAX;
	_topic = "";
}
Channel::~Channel()
{}

const std::string &Channel::getName() const {
	return _name;
}

void	Channel::setName(const std::string &name) {
	_name = name;
}

const std::string &Channel::getTopic() const {
	return _topic;
}

void	Channel::setTopic(const std::string &topic) {
	_topic = topic;
}

const std::string &Channel::getKey() const {
	return _key;
}

void	Channel::setKey(const std::string &key) {
	_key = key;
	_needKey = true;
}

void	Channel::removeKey() {
	_key.clear();
	_needKey = false;
}

bool	Channel::hasMember(int clientFd) const {
	for (size_t i = 0; i < _memberFds.size(); ++i) {
		if (_memberFds[i] == clientFd)
			return true;
	}
	return false;
}

const std::vector<int>	&Channel::getMemberFds() const {
	return _memberFds;
}

bool	Channel::isEmpty() const {
	return (_memberFds.size() == 0);
}

void	Channel::addMember(int clientFd) {
	if (hasMember(clientFd) == false)
		_memberFds.push_back(clientFd);
}

void	Channel::removeMember(int clientFd) {
	for(size_t i = 0; i < _memberFds.size(); ++i) {
		if (_memberFds[i] == clientFd)
			_memberFds.erase(_memberFds.begin() + i);
		if (hasOperator(clientFd))
			removeOperator(clientFd);
	}
}

bool	Channel::hasOperator(int clientFd) const {
	for (size_t i = 0; i < _operatorFds.size(); ++i) {
		if (_operatorFds[i] == clientFd)
			return true;
	}
	return false;
}

int		Channel::addOperator(int clientFd) {
	if (!hasMember(clientFd))
		return (OPERATOR_NOT_MEMBER);
	if (hasOperator(clientFd))
		return (OPERATOR_ALREADY_SET);
	_operatorFds.push_back(clientFd);
	return (OPERATOR_ADDED);
}

int		Channel::removeOperator(int clientFd) {
	for (size_t i = 0; i < _operatorFds.size(); ++i) {
		if (_operatorFds[i] == clientFd) {
			//if (_operatorFds.size() == 1)
			//	return (OPERATOR_LESS_THAN_ONE);
			_operatorFds.erase(_operatorFds.begin() + i);
			return (OPERATOR_REMOVED);
		}
	}
	return (OPERATOR_NOT_FOUND);
}

std::vector<int> Channel::getOperatorFds() const {
	return _operatorFds;
}

bool	Channel::hasInvite(int clientFd) const {
	for (size_t i = 0; i < _invitedFds.size(); ++i) {
		if (_invitedFds[i] == clientFd)
			return true;
	}
	return false;
}

int		Channel::addInvite(int clientFd) {
	if (hasMember(clientFd))
		return (INVITE_ALREADY_MEMBER);
	if (hasInvite(clientFd))
		return (INVITE_ALREADY_SET);
	_invitedFds.push_back(clientFd);
	return (INVITE_ADDED);
}

int 	Channel::removeInvite(int clientFd) {
	if (!hasInvite(clientFd))
		return (INVITE_NOT_EXIST);
	for (size_t i = 0; i < _invitedFds.size(); ++i) {
		if (_invitedFds[i] == clientFd) {
			_invitedFds.erase(_invitedFds.begin() + i);
			break;
		}
	}
	return (INVITE_REMOVED);
}

bool	Channel::isInviteOnly() const {
	return _inviteOnly;
}

void	Channel::setInviteOnly(bool value) {
	_inviteOnly = value;
}

bool	Channel::isTopicRestricted() const {
	return _topicRestricted;
}

void	Channel::setTopicRestricted(bool value) {
	_topicRestricted = value;
}

bool	Channel::isKeyNeeded() const {
	return _needKey;
}

void	Channel::setKeyNeeded(bool value) {
	_needKey = value;
}

bool	Channel::isNumLimited() const {
	return _numLimited;
}

void	Channel::setNumLimited(bool value) {
	_numLimited = value;
}

int		Channel::getMaxMemberNum() const {
	return _maxMemberNum;
}

void	Channel::setMaxMemberNum(int number) {
	_maxMemberNum = number;
	_numLimited = true;
}

void	Channel::removeMaxMemberNum() {
	_maxMemberNum = INT_MAX;
	_numLimited = false;
}
