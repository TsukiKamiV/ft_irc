#include "Client.hpp"

Client::Client() :
	_clientFd(-1),
	_ip(""),
	_nick(""),
	_username(""),
	_userHost(""),
	_hostField(""),
	_serverName(""),
	_realName(""),
	_prefix(""),
	_buffer(""),
	_sendBuffer(""),
	_registered(false),
	_passApprouved(false),
	_shouldDisconnect(false)
{}

Client::Client(int fd, const std::string &ip):
	_clientFd(fd),
	_ip(ip),
	_nick(""),
	_username(""),
	_userHost(""),
	_hostField(""),
	_serverName(""),
	_realName(""),
	_prefix(""),
	_buffer(""),
	_sendBuffer(""),
	_registered(false),
	_passApprouved(false),
	_shouldDisconnect(false)
{}

Client::~Client() {}

int	Client::getFd() const {
	return this->_clientFd;
}

void	Client::setFd(int fd) {
	this->_clientFd = fd;
}

const std::string	&Client::getIP() const {
	return this->_ip;
}

void	Client::setIP(const std::string &ip) {
	this->_ip = ip;
}

const std::string &Client::getNick() const {
	return this->_nick;
}

void	Client::setNick(const std::string &nick) {
	this->_nick = nick;
}

const std::string &Client::getUsername() const {
	return this->_username;
}

void	Client::setUsername(const std::vector<std::string> &params) {
	this->_username = params[0];
	this->_userHost = params[1];
	this->_serverName = params[2];
	this->_realName = params[3];
}

const std::string	&Client::getHostField() const {
	return this->_hostField;
}

void	Client::setHostField(const std::string &hostField) {
	this->_hostField = hostField;
}

const std::string	&Client::getUserHost() const {
	return this->_userHost;
}

const std::string 	&Client::getServerName() const {
	return this->_serverName;
}

const std::string	&Client::getRealName() const {
	return this->_realName;
}

std::string Client::getPrefix() const {
	std::string host;
	host = getHostField();
	if (host.empty())
		host = getIP();
	if (host.empty())
		host = "unknown";
	return (this->getNick() + "!" + this->getUsername() + "@" + host);
}

const std::string &Client::getBuffer() const {
	return this->_buffer;
}

std::string &Client::getBuffer() {
	return this->_buffer;
}

void	Client::setBuffer(const std::string &buffer) {
	this->_buffer = buffer;
}

const std::string &Client::getSendBuffer() const {
	return this->_sendBuffer;
}

std::string &Client::getSendBuffer() {
	return this->_sendBuffer;
}

void	Client::setSendBuffer(const std::string &sendBuffer) {
	this->_sendBuffer = sendBuffer;
}

void	Client::appendBuffer(const std::string &chunk) {
	_buffer += chunk;
}

void	Client::eraseBuffer(size_t len) {
	if (len >= _buffer.size())
		_buffer.clear();
	else
		_buffer.erase(0, len);
}

void	Client::appendSendBuffer(const std::string &chunk) {
	_sendBuffer += chunk;
}

void	Client::eraseSendBuffer(size_t len) {
	if (len >= _sendBuffer.size())
		_sendBuffer.clear();
	else
		_sendBuffer.erase(0, len);
}

bool	Client::isRegistered() const {
	return _registered;
}

void	Client::setRegistered(bool value) {
	this->_registered = value;
}

bool	Client::isPassApprouved() const {
	return _passApprouved;
}

void	Client::setPassApprouved(bool value) {
	_passApprouved = value;
}

bool	Client::shouldDisconnect() const {
	return _shouldDisconnect;
}

void	Client::setShouldDisconnect(bool value) {
	_shouldDisconnect = value;
}
