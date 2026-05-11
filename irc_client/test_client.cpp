#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

/*
** Minimal IRC test client
** Supported modes:
** 1. normal        : receive and print messages normally
** 2. sendlong      : automatically send extra-long channel messages
** 3. sendshortlong : send one shorter long message only
** 4. slow          : read server messages very slowly
** 5. noread        : connect but barely read anything
** 6. interactive   : receive normally and allow continued terminal input
** 7. summary       : receive messages and print only compact summaries
** 8. slowsummary   : read slowly and print compact summaries only
*/

/*
**./test_client 127.0.0.1 6667 pass aaa user normal #1
**./test_client 127.0.0.1 6667 pass bbb user slow #1
**./test_client 127.0.0.1 6667 pass ccc user sendlong #1
**./test_client 127.0.0.1 6667 pass hhh user sendshortlong #1
**./test_client 127.0.0.1 6667 pass ddd user noread #1
**./test_client 127.0.0.1 6667 pass eee user interactive #1
**./test_client 127.0.0.1 6667 pass fff user summary #1
**./test_client 127.0.0.1 6667 pass ggg user slowsummary #1
*/

static bool	g_running = true;

static void	printUsage(void)
{
	std::cout
		<< "Usage: ./test_client <host> <port> <password> <nick> <user> <mode> [channel]\n"
		<< "Modes:\n"
		<< "  normal        - receive and print messages normally\n"
		<< "  sendlong      - automatically send extra-long channel messages\n"
		<< "  sendshortlong - send one shorter long message only\n"
		<< "  slow          - read server messages slowly\n"
		<< "  noread        - barely read any server messages\n"
		<< "  interactive   - receive normally and allow continued input\n"
		<< "  summary       - receive and print compact message summaries\n"
		<< "  slowsummary   - read slowly and print compact summaries\n";
}

static bool	isValidMode(const std::string &mode)
{
	return (mode == "normal" || mode == "sendlong"
		|| mode == "sendshortlong" || mode == "slow"
		|| mode == "noread" || mode == "interactive"
		|| mode == "summary" || mode == "slowsummary");
}

static int	parsePort(const char *str)
{
	char	*end;
	long	port;

	errno = 0;
	end = NULL;
	port = std::strtol(str, &end, 10);
	if (errno == ERANGE || end == NULL || *end != '\0')
		throw std::runtime_error("invalid port");
	if (port < 1 || port > 65535)
		throw std::runtime_error("port out of range");
	return (static_cast<int>(port));
}

static int	setNonBlocking(int fd)
{
	int	flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (-1);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return (-1);
	return (0);
}

static int	connectToServer(const std::string &host, int port)
{
	int					fd;
	struct sockaddr_in	addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() failed");
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
	{
		close(fd);
		throw std::runtime_error("invalid IPv4 address");
	}
	if (connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1)
	{
		close(fd);
		throw std::runtime_error(std::string("connect() failed: ")
			+ std::strerror(errno));
	}
	if (setNonBlocking(fd) == -1)
	{
		close(fd);
		throw std::runtime_error("setNonBlocking() failed");
	}
	return (fd);
}

static void	sendAll(int fd, const std::string &msg)
{
	ssize_t				sent;
	size_t				offset;
	const char			*buf;
	size_t				len;

	offset = 0;
	buf = msg.c_str();
	len = msg.size();
	while (offset < len)
	{
		sent = send(fd, buf + offset, len - offset, 0);
		if (sent > 0)
		{
			offset += static_cast<size_t>(sent);
			continue ;
		}
		if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			usleep(10000);
			continue ;
		}
		throw std::runtime_error(std::string("send() failed: ")
			+ std::strerror(errno));
	}
}

static std::string	buildLongMessage(size_t repeatCount, int counter)
{
	std::string		base;
	std::string		msg;
	size_t			i;
	std::ostringstream	oss;

	base = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	msg = "";
	i = 0;
	while (i < repeatCount)
	{
		oss.str("");
		oss.clear();
		oss << "[BLOCK-" << counter << "-" << i << "]";
		msg += oss.str();
		msg += base;
		i++;
	}
	msg += "---LONG-MESSAGE-TEST---";
	i = 0;
	while (i < repeatCount)
	{
		oss.str("");
		oss.clear();
		oss << "[TAIL-" << counter << "-" << i << "]";
		msg += oss.str();
		msg += "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		i++;
	}
	msg += "---END---";
	return (msg);
}

static void	sendRegistration(int fd, const std::string &password,
	const std::string &nick, const std::string &user)
{
	sendAll(fd, "PASS " + password + "\r\n");
	sendAll(fd, "NICK " + nick + "\r\n");
	sendAll(fd, "USER " + user + " 0 * :" + user + "\r\n");
}

static void	sendJoin(int fd, const std::string &channel)
{
	if (!channel.empty())
		sendAll(fd, "JOIN " + channel + "\r\n");
}

static void	printReadableChunk(const std::string &chunk)
{
	if (!chunk.empty())
		std::cout << chunk << std::flush;
}

static std::string	extractTagValue(const std::string &line, const std::string &prefix)
{
	size_t	start;
	size_t	end;

	start = line.find(prefix);
	if (start == std::string::npos)
		return ("N/A");
	start += prefix.size();
	end = start;
	while (end < line.size() && line[end] != ']' && line[end] != ' ')
		end++;
	return (line.substr(start, end - start));
}

static std::string	buildHeadPreview(const std::string &line)
{
	size_t	previewLen;

	previewLen = 60;
	if (line.size() <= previewLen)
		return (line);
	return (line.substr(0, previewLen) + "...");
}

static std::string	buildTailPreview(const std::string &line)
{
	size_t	previewLen;

	previewLen = 60;
	if (line.size() <= previewLen)
		return (line);
	return ("..." + line.substr(line.size() - previewLen));
}

static void	printSummaryLine(const std::string &line)
{
	std::string	longId;
	std::string	blockId;
	std::string	tailId;
	std::string	endMark;

	longId = extractTagValue(line, "[LONG-");
	if (longId == "N/A")
		longId = extractTagValue(line, "[SHORTLONG-]");
	blockId = extractTagValue(line, "[BLOCK-");
	tailId = extractTagValue(line, "[TAIL-");
	if (line.find("---END---") != std::string::npos)
		endMark = "yes";
	else
		endMark = "no";
	std::cout << "[SUMMARY]"
		<< " long=" << longId
		<< " block=" << blockId
		<< " tail=" << tailId
		<< " len=" << line.size()
		<< " end=" << endMark
		<< std::endl;
	std::cout << "  head: " << buildHeadPreview(line) << std::endl;
	std::cout << "  tail: " << buildTailPreview(line) << std::endl;
}

static void	recvAndPrintNormal(int fd)
{
	char	buffer[1025];
	ssize_t	bytesRead;

	bytesRead = recv(fd, buffer, 1024, 0);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		printReadableChunk(std::string(buffer, bytesRead));
	}
	else if (bytesRead == 0)
	{
		std::cout << "\n[CLIENT] server closed connection.\n";
		g_running = false;
	}
	else if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
		std::cerr << "\n[CLIENT] recv failed: " << std::strerror(errno) << std::endl;
		g_running = false;
	}
}

static void	recvAndPrintSummary(int fd, std::string &recvBuffer)
{
	char		buffer[1025];
	ssize_t		bytesRead;
	size_t		pos;
	std::string	line;

	bytesRead = recv(fd, buffer, 1024, 0);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		recvBuffer.append(buffer, bytesRead);
		while (true)
		{
			pos = recvBuffer.find('\n');
			if (pos == std::string::npos)
				break ;
			line = recvBuffer.substr(0, pos);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			recvBuffer.erase(0, pos + 1);
			printSummaryLine(line);
		}
	}
	else if (bytesRead == 0)
	{
		std::cout << "\n[CLIENT] server closed connection.\n";
		g_running = false;
	}
	else if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
		std::cerr << "\n[CLIENT] recv failed: " << std::strerror(errno) << std::endl;
		g_running = false;
	}
}

static void	recvAndPrintSlowSummary(int fd, std::string &recvBuffer)
{
	char		buffer[65];
	ssize_t		bytesRead;
	size_t		pos;
	std::string	line;

	bytesRead = recv(fd, buffer, 64, 0);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		recvBuffer.append(buffer, bytesRead);
		while (true)
		{
			pos = recvBuffer.find('\n');
			if (pos == std::string::npos)
				break ;
			line = recvBuffer.substr(0, pos);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			recvBuffer.erase(0, pos + 1);
			printSummaryLine(line);
		}
	}
	else if (bytesRead == 0)
	{
		std::cout << "\n[CLIENT] server closed connection.\n";
		g_running = false;
	}
	else if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
		std::cerr << "\n[CLIENT] recv failed: " << std::strerror(errno) << std::endl;
		g_running = false;
	}
}

static void	recvAndPrintSlow(int fd)
{
	char	buffer[17];
	ssize_t	bytesRead;

	bytesRead = recv(fd, buffer, 16, 0);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		printReadableChunk(std::string(buffer, bytesRead));
	}
	else if (bytesRead == 0)
	{
		std::cout << "\n[CLIENT] server closed connection.\n";
		g_running = false;
	}
	else if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
		std::cerr << "\n[CLIENT] recv failed: " << std::strerror(errno) << std::endl;
		g_running = false;
	}
}

static void	runNormalLoop(int fd)
{
	while (g_running)
	{
		recvAndPrintNormal(fd);
		usleep(10000);
	}
}

static void	runSummaryLoop(int fd)
{
	std::string	recvBuffer;

	recvBuffer = "";
	while (g_running)
	{
		recvAndPrintSummary(fd, recvBuffer);
		usleep(10000);
	}
}

static void	runSlowSummaryLoop(int fd)
{
	std::string	recvBuffer;

	recvBuffer = "";
	while (g_running)
	{
		usleep(200000);
		recvAndPrintSlowSummary(fd, recvBuffer);
	}
}

static void	runSlowLoop(int fd)
{
	while (g_running)
	{
		usleep(2000000);
		recvAndPrintSlow(fd);
	}
}

static void	runNoReadLoop(void)
{
	while (g_running)
		usleep(2000000);
}

static void	handleInteractiveInput(int fd, std::string &inputBuffer)
{
	char	buffer[1025];
	ssize_t	bytesRead;
	size_t	pos;
	std::string	line;

	bytesRead = read(0, buffer, 1024);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		inputBuffer.append(buffer, bytesRead);
		while (true)
		{
			pos = inputBuffer.find('\n');
			if (pos == std::string::npos)
				break ;
			line = inputBuffer.substr(0, pos);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			inputBuffer.erase(0, pos + 1);
			if (line == "/quit")
			{
				g_running = false;
				return ;
			}
			sendAll(fd, line + "\r\n");
		}
	}
	else if (bytesRead == 0)
		g_running = false;
	else if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
	{
		std::cerr << "\n[CLIENT] stdin read failed: "
			<< std::strerror(errno) << std::endl;
		g_running = false;
	}
}

static void	runInteractiveLoop(int fd)
{
	struct pollfd	fds[2];
	int			pollRet;
	std::string	inputBuffer;

	inputBuffer = "";
	std::cout << "[CLIENT] interactive mode ready. Type /quit to exit." << std::endl;
	while (g_running)
	{
		fds[0].fd = 0;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = fd;
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		pollRet = poll(fds, 2, 500);
		if (pollRet == -1)
		{
			if (errno == EINTR)
				continue ;
			throw std::runtime_error(std::string("poll() failed: ")
				+ std::strerror(errno));
		}
		if (pollRet == 0)
			continue ;
		if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			std::cout << "\n[CLIENT] server connection closed.\n";
			g_running = false;
			continue ;
		}
		if (fds[1].revents & POLLIN)
			recvAndPrintNormal(fd);
		if (!g_running)
			break ;
		if (fds[0].revents & POLLIN)
			handleInteractiveInput(fd, inputBuffer);
	}
}

static void	runSendLongLoop(int fd, const std::string &channel)
{
	std::string	longMsg;
	int			counter;
	std::ostringstream oss;

	counter = 1;
	while (g_running)
	{
		longMsg = buildLongMessage(64, counter);
		oss.str("");
		oss.clear();
		oss << "PRIVMSG " << channel << " :[LONG-" << counter << "] " << longMsg << "\r\n";
		sendAll(fd, oss.str());
		counter++;
		usleep(300000);
		recvAndPrintNormal(fd);
	}
}

static void	runSendShortLongOnce(int fd, const std::string &channel)
{
	std::string		longMsg;
	std::ostringstream	oss;

	longMsg = buildLongMessage(16, 1);
	oss.str("");
	oss.clear();
	oss << "PRIVMSG " << channel << " :[SHORTLONG-1] " << longMsg << "\r\n";
	sendAll(fd, oss.str());
	recvAndPrintNormal(fd);
	while (g_running)
	{
		recvAndPrintNormal(fd);
		usleep(10000);
	}
}

int	main(int argc, char **argv)
{
	std::string	host;
	int			port;
	std::string	password;
	std::string	nick;
	std::string	user;
	std::string	mode;
	std::string	channel;
	int			fd;

	if (argc < 7 || argc > 8)
	{
		printUsage();
		return (1);
	}
	try
	{
		host = argv[1];
		port = parsePort(argv[2]);
		password = argv[3];
		nick = argv[4];
		user = argv[5];
		mode = argv[6];
		channel = "";
		if (argc == 8)
			channel = argv[7];
		if (!isValidMode(mode))
			throw std::runtime_error("invalid mode");
		fd = connectToServer(host, port);
		std::cout << "[CLIENT] connected to " << host << ":" << port << std::endl;
		sendRegistration(fd, password, nick, user);
		usleep(300000);
		sendJoin(fd, channel);
		usleep(300000);
		if (mode == "normal")
			runNormalLoop(fd);
		else if (mode == "summary")
			runSummaryLoop(fd);
		else if (mode == "slowsummary")
			runSlowSummaryLoop(fd);
		else if (mode == "slow")
			runSlowLoop(fd);
		else if (mode == "noread")
			runNoReadLoop();
		else if (mode == "sendlong")
		{
			if (channel.empty())
				throw std::runtime_error("sendlong mode requires channel");
			runSendLongLoop(fd, channel);
		}
		else if (mode == "sendshortlong")
		{
			if (channel.empty())
				throw std::runtime_error("sendshortlong mode requires channel");
			runSendShortLongOnce(fd, channel);
		}
		else if (mode == "interactive")
			runInteractiveLoop(fd);
		close(fd);
	}
	catch (const std::exception &e)
	{
		std::cerr << "[CLIENT ERROR] " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
