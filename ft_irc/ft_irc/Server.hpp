#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Client.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

#define	MAX_CONNECTION 100

struct parseMessage {
	std::string					command;
	std::vector<std::string> 	params;
	bool						hasTrailingParam;
	
	parseMessage(): command(), params(), hasTrailingParam(false) {}
};
/**
 *@brief 总控服务器
 *通常负责：
 **保存端口和密码
 **创建监听socket
 **初始化non-blocking
 **管理POLL
 **接受新连接
 **保存所有在线客户端
 **保存所有频道
 **主事件循环
 **分发命令
 **清理断线用户
 */
class Server {
public:
	Server(long port, const std::string &password);
	//服务器作为总控类，通常不应该被复制
	~Server();
	
	//ServerCore.cpp
	void	initServer();
	void	run();
	void	closeFds();
	
private:
	//Copy and reassignment not allowed
	Server(const Server &other);
	Server &operator=(const Server &other);
	
	long						_port;
	std::string					_password;
	int							_serverSocketFd;
	static bool					_signal;
	std::vector<Client> 		_clients;
	std::vector<Channel>		_channels;
	std::vector<pollfd> 		_fds;
	
	//ServerCore.cpp
	int		createListeningSocket(long port);
	int		setNonBlocking(int fd);
	void	handlePollEvents();
	void	acceptNewClient();
	bool	handleClientEvent(size_t index);
	void	removeClient(size_t index, const std::string &reason);

	//ServerAuth.cpp
	void	handlePass(size_t clientIndex, const parseMessage &msg);
	void	handleNick(size_t clientIndex, const parseMessage &msg);
	void	handleUser(size_t clientIndex, const parseMessage &msg);
	void	checkRegistration(size_t clientIndex);
	bool	isClientRegistered(size_t clientIndex) const;
	int		findClientIndexByNick(const std::string &nick) const;
	
	//ServerChannel.cpp
	void	handleJoin(size_t clientIndex, const parseMessage &msg);
	void	removeClientFromChannels(size_t index, int clientFd, const std::string &reason);
	int		findChannelIndex(const std::string &channelName) const;
	void	removeChannel(int channelIndex);
	std::string	buildNameReplyList(int channelIndex) const;
	
	//ServerIO.cpp
	void	handleClientBuffer(size_t index, const std::string &chunk);
	void	handleSendBuffer(size_t index, const std::string &chunk);
	void	updateClientPollEvent(size_t clientIndex);
	void	processLine(size_t clientIndex, const std::string &line);
	parseMessage	parseLine(const std::string &line);
	void	sendToClient(size_t clientIndex, const std::string &message);
	//void	sendToClientBuffer(size_t clientIndex, const std::string &message);
	void	channelBroadcast(int channelIndex, const std::string &message);
	void	channelBroadcastExcept(int channelIndex, const std::string &message, int exceptFd);
	void	broadcastNickChange(int clientFd, const std::string &oldNick, const std::string &newNick);
	std::string	getReplyTarget(size_t clientIndex) const;
	
	//Commands.cpp
	void	handleMode(size_t clientIndex, const parseMessage &msg);
	void	handleKick(size_t clientIndex, const parseMessage &msg);
	void	handleInvite(size_t clientIndex, const parseMessage &msg);
	void	handleTopic(size_t clientIndex, const parseMessage &msg);
	void	handlePrivmsg(size_t clientIndex, const parseMessage &msg);
	void	handlePing(size_t clientIndex, const parseMessage &msg);
	void	handlePong(size_t clientIndex, const parseMessage &msg);
	void	handleQuit(size_t clientIndex, const parseMessage &msg);
	
	//Mode.cpp
	void	operatorManager(size_t clientIndex, const std::string &modeString, int channelIndex, int targetFd, const std::string &targetNick);
	void	inviteOnlyManager(size_t clientIndex, const std::string &modeString, int channelIndex);
	void	topicManager(size_t clientIndex, const std::string &modeString, int channelIndex);
	void	keyManager(size_t clientIndex, const std::string &modeString, const std::string &key, int channelIndex);
	void	maxMemberManager(size_t clientIndex, const std::string &modeString, int maxNumber, int channelIndex);
	bool	checkModeParams(size_t clientIndex, const parseMessage &msg);
	
};

#endif
