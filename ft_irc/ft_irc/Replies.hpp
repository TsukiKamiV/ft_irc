#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

namespace Replies {


inline std::string ERR_NOSUCHNICK(
								  const std::string &server,
								  const std::string &target,
								  const std::string &nick)
{
	return ":" + server + " 401 " + target + " " + nick
	+ " :No such nick/channel\r\n";
}

inline std::string ERR_NOSUCHCHANNEL(
									 const std::string &server,
									 const std::string &target,
									 const std::string &channel)
{
	return ":" + server + " 403 " + target + " " + channel
	+ " :No such channel\r\n";
}

inline std::string ERR_NOTEXTTOSEND(
									const std::string &server,
									const std::string &target)
{
	return ":" + server + " 412 " + target + " :No text to send\r\n";
}

inline std::string ERR_UNKNOWNCOMMAND(
									  const std::string &server,
									  const std::string &target,
									  const std::string &command)
{
	return ":" + server + " 421 " + target + " " + command
	+ " :Unknown command\r\n";
}

inline std::string ERR_NONICKNAMEGIVEN(
									   const std::string &server,
									   const std::string &target)
{
	return ":" + server + " 431 " + target + " :No nickname given\r\n";
}

inline std::string ERR_ERRONEUSNICKNAME(
										const std::string &server,
										const std::string &target,
										const std::string &nick)
{
	return ":" + server + " 432 " + target + " " + nick
	+ " :Erroneous nickname\r\n";
}

inline std::string ERR_NICKNAMEINUSE(
									 const std::string &server,
									 const std::string &target,
									 const std::string &nick)
{
	return ":" + server + " 433 " + target + " " + nick
	+ " :Nickname is already in use\r\n";
}

inline std::string ERR_USERNOTINCHANNEL(
										const std::string &server,
										const std::string &target,
										const std::string &nick,
										const std::string &channel)
{
	return ":" + server + " 441 " + target + " " + nick + " " + channel
	+ " :They aren't on that channel\r\n";
}

inline std::string ERR_NOTONCHANNEL(
									const std::string &server,
									const std::string &target,
									const std::string &channel)
{
	return ":" + server + " 442 " + target + " " + channel
	+ " :You're not on that channel\r\n";
}

inline std::string ERR_USERONCHANNEL(
									 const std::string &server,
									 const std::string &target,
									 const std::string &nick,
									 const std::string &channel)
{
	return ":" + server + " 443 " + target + " " + nick + " " + channel
	+ " :is already on channel\r\n";
}

inline std::string ERR_NOTREGISTERED(
									 const std::string &server,
									 const std::string &target)
{
	return ":" + server + " 451 " + target + " :You have not registered\r\n";
}

inline std::string ERR_NEEDMOREPARAMS(
									  const std::string &server,
									  const std::string &target,
									  const std::string &command)
{
	return ":" + server + " 461 " + target + " " + command
	+ " :Not enough parameters\r\n";
}

inline std::string ERR_ALREADYREGISTERED(
										 const std::string &server,
										 const std::string &target)
{
	return ":" + server + " 462 " + target + " :You may not reregister\r\n";
}

inline std::string ERR_PASSWDMISMATCH(
									  const std::string &server,
									  const std::string &target)
{
	return ":" + server + " 464 " + target + " :Password incorrect\r\n";
}

inline std::string ERR_CHANNELISFULL(
									 const std::string &server,
									 const std::string &target,
									 const std::string &channel)
{
	return ":" + server + " 471 " + target + " " + channel
	+ " :Cannot join channel (+l)\r\n";
}

inline std::string ERR_UNKNOWNMODE(
								   const std::string &server,
								   const std::string &target,
								   const std::string &mode)
{
	return ":" + server + " 472 " + target + " " + mode
	+ " :is unknown mode char to me\r\n";
}

inline std::string ERR_INVITEONLYCHAN(
									  const std::string &server,
									  const std::string &target,
									  const std::string &channel)
{
	return ":" + server + " 473 " + target + " " + channel
	+ " :Cannot join channel (+i)\r\n";
}

inline std::string ERR_BADCHANNELKEY(
									 const std::string &server,
									 const std::string &target,
									 const std::string &channel)
{
	return ":" + server + " 475 " + target + " " + channel
	+ " :Cannot join channel (+k)\r\n";
}

inline std::string ERR_BADCHANMASK(
								   const std::string &server,
								   const std::string &target,
								   const std::string &channel)
{
	return ":" + server + " 476 " + target + " " + channel
	+ " :Bad Channel Mask\r\n";
}

inline std::string ERR_CHANOPRIVSNEEDED(
										const std::string &server,
										const std::string &target,
										const std::string &channel)
{
	return ":" + server + " 482 " + target + " " + channel
	+ " :You're not channel operator\r\n";
}

inline std::string RPL_WELCOME(
							   const std::string &server,
							   const std::string &target)
{
	return ":" + server + " 001 " + target
	+ " :Welcome to the IRC Network " + target + "\r\n";
}

inline std::string RPL_NOTOPIC(
							   const std::string &server,
							   const std::string &target,
							   const std::string &channel)
{
	return ":" + server + " 331 " + target + " " + channel
	+ " :No topic is set\r\n";
}

inline std::string RPL_TOPIC(
							 const std::string &server,
							 const std::string &target,
							 const std::string &channel,
							 const std::string &topic)
{
	return ":" + server + " 332 " + target + " " + channel
	+ " :" + topic + "\r\n";
}

inline std::string RPL_INVITING(
								const std::string &server,
								const std::string &target,
								const std::string &nick,
								const std::string &channel)
{
	return ":" + server + " 341 " + target + " " + nick
	+ " " + channel + "\r\n";
}

inline std::string RPL_NAMREPLY(
								const std::string &server,
								const std::string &target,
								const std::string &channel,
								const std::string &nickList)
{
	return ":" + server + " 353 " + target + " = " + channel
	+ " :" + nickList + "\r\n";
}

inline std::string RPL_ENDOFNAMES(
								  const std::string &server,
								  const std::string &target,
								  const std::string &channel)
{
	return ":" + server + " 366 " + target + " " + channel
	+ " :End of /NAMES list\r\n";
}

inline std::string RPL_NICK(
							const std::string &prefix,
							const std::string &newNick)
{
	return ":" + prefix + " NICK " + newNick + "\r\n";
}

inline std::string RPL_JOIN(
							const std::string &prefix,
							const std::string &channel)
{
	return ":" + prefix + " JOIN " + channel + "\r\n";
}

inline std::string RPL_PART(
							const std::string &prefix,
							const std::string &channel,
							const std::string &reason)
{
	if (reason.empty())
		return ":" + prefix + " PART " + channel + "\r\n";
	return ":" + prefix + " PART " + channel + " :" + reason + "\r\n";
}

inline std::string RPL_KICK(
							const std::string &prefix,
							const std::string &channel,
							const std::string &target,
							const std::string &reason)
{
	if (reason.empty())
		return ":" + prefix + " KICK " + channel + " " + target + "\r\n";
	return ":" + prefix + " KICK " + channel + " " + target
	+ " :" + reason + "\r\n";
}

inline std::string RPL_QUIT(
							const std::string &prefix,
							const std::string &reason)
{
	return ":" + prefix + " QUIT :" + reason + "\r\n";
}

inline std::string RPL_PRIVMSG(
							   const std::string &prefix,
							   const std::string &target,
							   const std::string &message)
{
	return ":" + prefix + " PRIVMSG " + target + " :" + message + "\r\n";
}

inline std::string RPL_INVITE(
							  const std::string &prefix,
							  const std::string &target,
							  const std::string &channel)
{
	return ":" + prefix + " INVITE " + target + " " + channel + "\r\n";
}

inline std::string RPL_PONG(
							const std::string &server,
							const std::string &token)
{
	return ":" + server + " PONG " + server + " :" + token + "\r\n";
}

inline std::string RPL_TOPICCMD(
								const std::string &prefix,
								const std::string &channel,
								const std::string &topic)
{
	return ":" + prefix + " TOPIC " + channel + " :" + topic + "\r\n";
}

inline std::string RPL_MODE(
							const std::string &prefix,
							const std::string &channel,
							const std::string &mode,
							const std::string &param)
{
	if (param.empty())
		return ":" + prefix + " MODE " + channel + " " + mode + "\r\n";
	return ":" + prefix + " MODE " + channel + " " + mode
	+ " " + param + "\r\n";
}

}

#endif
