#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client {
private:
	int 		_clientFd;
	std::string _ip;
	std::string	_nick;
	std::string _username;
	std::string	_userHost;
	std::string _hostField;
	std::string _serverName;
	std::string _realName;
	std::string	_prefix;
	std::string	_buffer;
	std::string _sendBuffer;
	bool		_registered;
	bool		_passApprouved;
	bool		_shouldDisconnect;
	
public:
	Client();
	Client(int fd, const std::string &ip);
	~Client();
	
	/*======getters & setters=========*/
	int			getFd() const;
	void				setFd(int fd);
	
	const std::string	&getIP() const;
	void				setIP(const std::string &ip);
	
	const std::string	&getNick() const;
	void				setNick(const std::string &nick);
	
	const std::string	&getUsername() const;
	const std::string	&getUserHost() const;
	const std::string 	&getServerName() const;
	const std::string	&getRealName() const;
	void				setUsername(const std::vector<std::string> &params);
	
	const std::string	&getHostField() const;
	void				setHostField(const std::string &hostField);
	
	std::string	getPrefix() const;
	//void				setPrefix(const std::string &nick, const std::string &username, const std::string &host);
	
	const std::string	&getBuffer() const;
	std::string			&getBuffer();
	void				setBuffer(const std::string &buffer);
	
	const std::string	&getSendBuffer() const;
	std::string			&getSendBuffer();
	void				setSendBuffer(const std::string &sendBuffer);
	
	bool				shouldDisconnect() const;
	void				setShouldDisconnect(bool value);
	
	/*================================*/
	
	void	appendBuffer(const std::string &chunk);
	void	eraseBuffer(size_t len);
	
	void	appendSendBuffer(const std::string &chunk);
	void	eraseSendBuffer(size_t len);
	
	/*============Registration and password bool=========*/
	bool	isRegistered() const;
	void	setRegistered(bool value);
	
	bool	isPassApprouved() const;
	void	setPassApprouved(bool value);
	/*===================================================*/
	
	
};

#endif
