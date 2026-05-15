#include "Client.hpp"

Client::Client()
: _clientFd(-1),
_ip(""),
_nick(""),
_username(""),
_userHost(""),
_hostField(""),
_serverName(""),
_realName(""),
_buffer(""),
_sendBuffer(""),
_registered(false),
_passApprouved(false),
_shouldDisconnect(false)
{}

Client::Client(int fd, const std::string &ip)
: _clientFd(fd),
_ip(ip),
_nick(""),
_username(""),
_userHost(""),
_hostField(""),
_serverName(""),
_realName(""),
_buffer(""),
_sendBuffer(""),
_registered(false),
_passApprouved(false),
_shouldDisconnect(false)
{}

Client::~Client() {}

int	Client::getFd() const {
	return _clientFd;
}

void	Client::setFd(int fd) {
	_clientFd = fd;
}

const std::string	&Client::getIP() const {
	return _ip;
}

void	Client::setIP(const std::string &ip) {
	_ip = ip;
}

const std::string	&Client::getNick() const {
	return _nick;
}

void	Client::setNick(const std::string &n) {
	_nick = n;
}

const std::string	&Client::getUsername() const {
	return _username;
}

const std::string	&Client::getUserHost() const {
	return _userHost;
}

const std::string	&Client::getServerName() const {
	return _serverName;
}

const std::string	&Client::getRealName() const {
	return _realName;
}

void	Client::setUsername(const std::vector<std::string> &params) {
	_username = params[0];
	_userHost = params[1];
	_serverName = params[2];
	_realName = params[3];
}

const std::string	&Client::getHostField() const {
	return _hostField;
}

void	Client::setHostField(const std::string &hostField) {
	_hostField = hostField;
}

std::string	Client::getPrefix() const {
	std::string host = _hostField.empty() ? (_ip.empty() ? "unknown" : _ip) : _hostField;
	return (_nick + "!" + _username + "@" + host);
}

const std::string	&Client::getBuffer() const {
	return _buffer;
}

std::string	&Client::getBuffer() {
	return _buffer;
}

void	Client::setBuffer(const std::string &b) {
	_buffer = b;
}

const std::string	&Client::getSendBuffer() const {
	return _sendBuffer;
}

std::string	&Client::getSendBuffer() {
	return _sendBuffer;
}

void	Client::setSendBuffer(const std::string &s) {
	_sendBuffer = s;
}

bool	Client::shouldDisconnect() const {
	return _shouldDisconnect;
}

void	Client::setShouldDisconnect(bool value) {
	_shouldDisconnect = value;
}

void	Client::appendBuffer(const std::string &chunk) {
	_buffer += chunk;
}

void	Client::appendSendBuffer(const std::string &chunk) {
	_sendBuffer += chunk;
}

void	Client::eraseBuffer(size_t len) {
	if (len >= _buffer.size())
		_buffer.clear();
	else
		_buffer.erase(0, len);
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
	_registered = value;
}

bool	Client::isPassApprouved() const {
	return _passApprouved;
}

void	Client::setPassApprouved(bool value) {
	_passApprouved = value;
}
