#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <vector>
#include <string>
#include <limits>

//#include "Client.hpp"
//#include "Server.hpp"

class Channel {
public:
	Channel(const std::string &name);
	~Channel();
	
	enum e_operator_result {
		OPERATOR_NOT_MEMBER = -1,
		OPERATOR_ALREADY_SET = 0,
		OPERATOR_ADDED = 1,
		OPERATOR_REMOVED = 2,
		OPERATOR_NOT_FOUND = 3,
		OPERATOR_LESS_THAN_ONE = 4
	};
	
	enum e_invite_result {
		INVITE_NOT_EXIST = -2,
		INVITE_ALREADY_MEMBER = -1,
		INVITE_ALREADY_SET = 0,
		INVITE_ADDED = 1,
		INVITE_REMOVED = 2
	};
	
	const 	std::string &getName() const;
	void	setName(const std::string &name);
	
	const 	std::string &getTopic() const;
	void	setTopic(const std::string &topic);
	
	const	std::string &getKey() const;
	void	setKey(const std::string &key);
	void	removeKey();
	
	bool	hasMember(int clientFd) const;
	void	addMember(int clientFd);
	void	removeMember(int clientFd);
	bool	isEmpty() const;
	const std::vector<int> &getMemberFds() const;
	
	bool	hasOperator(int clientFd) const;
	int		addOperator(int clientFd);
	int		removeOperator(int clientFd);
	std::vector<int> getOperatorFds() const;
	
	bool	hasInvite(int clientFd) const;
	int		addInvite(int clientFd);
	int		removeInvite(int clientFd);
	
	bool	isInviteOnly() const;
	void	setInviteOnly(bool value);
	
	bool	isTopicRestricted() const;
	void	setTopicRestricted(bool value);
	
	bool	isKeyNeeded() const;
	void	setKeyNeeded(bool value);
	
	bool	isNumLimited() const;
	void	setNumLimited(bool value);
	
	int		getMaxMemberNum() const;
	void	setMaxMemberNum(int number);
	void	removeMaxMemberNum();
	
	
	//const std::vector<int> &getOperator() const;
	//void	setOperator(const int operatorFd);
	
private:
	bool		_inviteOnly;
	bool		_topicRestricted;
	bool		_needKey;
	bool		_numLimited;
	std::string _name;
	std::string _topic;
	std::string _key;
	int			_maxMemberNum;
	std::vector<int> _memberFds;
	std::vector<int> _operatorFds;
	std::vector<int> _invitedFds;
	
};

#endif
