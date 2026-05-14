#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

namespace Replies
{
/* ==================== general tools ==================== */

inline std::string	makePrefix(const std::string &serverName)
{
	return (":" + serverName);
}

inline std::string	makeNumericReply(const std::string &serverName,
									 const std::string &code,
									 const std::string &target,
									 const std::string &params,
									 const std::string &text)
{
	std::string	reply;
	
	reply = ":" + serverName + " " + code;
	if (!target.empty())
		reply += " " + target;
	if (!params.empty())
		reply += " " + params;
	if (!text.empty())
		reply += " :" + text;
	reply += "\r\n";
	return (reply);
}

inline std::string	makeCommandReply(const std::string &prefix,
									 const std::string &command,
									 const std::string &params,
									 const std::string &trailing)
{
	std::string	reply;
	
	reply = ":" + prefix + " " + command;
	if (!params.empty())
		reply += " " + params;
	if (!trailing.empty())
		reply += " :" + trailing;
	reply += "\r\n";
	return (reply);
}

/* ==================== Registration and basic errors ==================== */

inline std::string	RPL_WELCOME(const std::string &serverName,
								const std::string &targetNick)
{
	return (makeNumericReply(serverName, "001", targetNick, "",
							 "Welcome " + targetNick + " to the ft_irc network"));
}

inline std::string	ERR_NOMOTD(const std::string &serverName,
							   const std::string &targetNick)
{
	return (makeNumericReply(serverName, "422", targetNick, "",
							 "MOTD File is missing"));
}

inline std::string	RPL_ENDOFMOTD(const std::string &serverName,
								  const std::string &targetNick)
{
	return (makeNumericReply(serverName, "376", targetNick, "",
							 "End of /MOTD command"));
}

inline std::string	ERR_UNKNOWNCOMMAND(const std::string &serverName,
									   const std::string &targetNick,
									   const std::string &command)
{
	return (makeNumericReply(serverName, "421", targetNick, command,
							 "Unknown command"));
}

inline std::string	ERR_NONICKNAMEGIVEN(const std::string &serverName,
										const std::string &targetNick)
{
	return (makeNumericReply(serverName, "431", targetNick, "",
							 "No nickname given"));
}

inline std::string	ERR_ERRONEUSNICKNAME(const std::string &serverName,
										 const std::string &targetNick,
										 const std::string &nick)
{
	return (makeNumericReply(serverName, "432", targetNick, nick, "Erroneous nickname"));
}

inline std::string	ERR_NICKNAMEINUSE(const std::string &serverName,
									  const std::string &targetNick,
									  const std::string &nick)
{
	return (makeNumericReply(serverName, "433", targetNick, nick,
							 "Nickname is already in use"));
}

inline std::string	ERR_NOTREGISTERED(const std::string &serverName,
									  const std::string &targetNick)
{
	return (makeNumericReply(serverName, "451", targetNick, "",
							 "You have not registered"));
}

inline std::string	ERR_NEEDMOREPARAMS(const std::string &serverName,
									   const std::string &targetNick,
									   const std::string &command)
{
	return (makeNumericReply(serverName, "461", targetNick, command,
							 "Not enough parameters"));
}

inline std::string	ERR_ALREADYREGISTERED(const std::string &serverName,
										  const std::string &targetNick)
{
	return (makeNumericReply(serverName, "462", targetNick, "",
							 "You may not reregister"));
}

inline std::string	ERR_PASSWDMISMATCH(const std::string &serverName,
									   const std::string &targetNick)
{
	return (makeNumericReply(serverName, "464", targetNick, "",
							 "Password is incorrect"));
}

/* ==================== NICK / USER / PRIVMSG  ==================== */

inline std::string	ERR_NOSUCHNICK(const std::string &serverName,
								   const std::string &targetNick,
								   const std::string &nick)
{
	return (makeNumericReply(serverName, "401", targetNick, nick,
							 "No such nick/channel"));
}

inline std::string	ERR_NOSUCHCHANNEL(const std::string &serverName,
									  const std::string &targetNick,
									  const std::string &channel)
{
	return (makeNumericReply(serverName, "403", targetNick, channel,
							 "No such channel"));
}

inline std::string	ERR_USERONCHANNEL(const std::string &serverName,
									  const std::string &targetNick,
									  const std::string &nick,
									  const std::string &channel)
{
	return (makeNumericReply(serverName, "443", targetNick,
							 nick + " " + channel, "is already on channel"));
}

inline std::string	ERR_BADCHANMASK(const std::string &serverName, const std::string &targetNick, const std::string &channel)
{
	return (makeNumericReply(serverName, "476", targetNick, channel, "Bad channel Mask"));
}

inline std::string	ERR_CANNOTSENDTOCHAN(const std::string &serverName,
										 const std::string &targetNick,
										 const std::string &channel)
{
	return (makeNumericReply(serverName, "404", targetNick, channel,
							 "Cannot send to channel"));
}

inline std::string	ERR_NOTONCHANNEL(const std::string &serverName,
									 const std::string &targetNick,
									 const std::string &channel)
{
	return (makeNumericReply(serverName, "442", targetNick, channel,
							 "You're not on that channel"));
}

inline std::string	ERR_USERNOTINCHANNEL(const std::string &serverName,
										 const std::string &targetNick,
										 const std::string &nick,
										 const std::string &channel)
{
	return (makeNumericReply(serverName, "441", targetNick,
							 nick + " " + channel, "They aren't on that channel"));
}

inline std::string	ERR_NOTEXTTOSEND(const std::string &serverName, const std::string &targetNick)
{
	return (makeNumericReply(serverName, "412", targetNick, "",
							 "No text to send"));
}

/* ==================== JOIN / Channel restriction ==================== */

inline std::string	ERR_TOOMANYCHANNELS(const std::string &serverName,
										const std::string &targetNick,
										const std::string &channel)
{
	return (makeNumericReply(serverName, "405", targetNick, channel,
							 "You have joined too many channels"));
}

inline std::string	ERR_CHANNELISFULL(const std::string &serverName,
									  const std::string &targetNick,
									  const std::string &channel)
{
	return (makeNumericReply(serverName, "471", targetNick, channel,
							 "Cannot join channel (+l)"));
}

inline std::string	ERR_INVITEONLYCHAN(const std::string &serverName,
									   const std::string &targetNick,
									   const std::string &channel)
{
	return (makeNumericReply(serverName, "473", targetNick, channel,
							 "Cannot join channel (+i)"));
}

inline std::string	ERR_BADCHANNELKEY(const std::string &serverName,
									  const std::string &targetNick,
									  const std::string &channel)
{
	return (makeNumericReply(serverName, "475", targetNick, channel,
							 "Cannot join channel (+k)"));
}

/* ==================== operator / MODE author ==================== */

inline std::string	ERR_CHANOPRIVSNEEDED(const std::string &serverName,
										 const std::string &targetNick,
										 const std::string &channel)
{
	return (makeNumericReply(serverName, "482", targetNick, channel,
							 "You're not channel operator"));
}

/* ==================== TOPIC  ==================== */

inline std::string	RPL_NOTOPIC(const std::string &serverName,
								const std::string &targetNick,
								const std::string &channel)
{
	return (makeNumericReply(serverName, "331", targetNick, channel,
							 "No topic is set"));
}

inline std::string	RPL_TOPIC(const std::string &serverName,
							  const std::string &targetNick,
							  const std::string &channel,
							  const std::string &topic)
{
	return (makeNumericReply(serverName, "332", targetNick, channel,
							 topic));
}

inline std::string	RPL_TOPICCMD(const std::string &prefix,
								 const std::string &channel,
								 const std::string &topic)
{
	return (makeCommandReply(prefix, "TOPIC", channel, topic));
}

/* ==================== NAMES / JOIN  ==================== */

inline std::string	RPL_NAMREPLY(const std::string &serverName,
								 const std::string &targetNick,
								 const std::string &channel,
								 const std::string &users)
{
	return (makeNumericReply(serverName, "353", targetNick,
							 "= " + channel, users));
}

inline std::string	RPL_ENDOFNAMES(const std::string &serverName,
								   const std::string &targetNick,
								   const std::string &channel)
{
	return (makeNumericReply(serverName, "366", targetNick, channel,
							 "End of /NAMES list"));
}

/* ==================== IRC commands ==================== */

inline std::string	RPL_JOIN(const std::string &prefix,
							 const std::string &channel)
{
	return (makeCommandReply(prefix, "JOIN", channel, ""));
}

inline std::string RPL_PART(const std::string &prefix,
							const std::string &channel,
							const std::string &reason)
{
	return (makeCommandReply(prefix, "PART", channel, reason));
}

inline std::string	RPL_PRIVMSG(const std::string &prefix,
								const std::string &target,
								const std::string &message)
{
	return (makeCommandReply(prefix, "PRIVMSG", target, message));
}

inline std::string	RPL_NOTICE(const std::string &prefix,
							   const std::string &target,
							   const std::string &message)
{
	return (makeCommandReply(prefix, "NOTICE", target, message));
}

inline std::string	RPL_QUIT(const std::string &prefix,
							 const std::string &reason)
{
	return (makeCommandReply(prefix, "QUIT", "", reason));
}

inline std::string	RPL_INVITING(const std::string &serverName,
								 const std::string &targetNick,
								 const std::string &invitedNick,
								 const std::string &channel)
{
	return (makeNumericReply(serverName, "341", targetNick, invitedNick + " " + channel, ""));
}

inline std::string	RPL_INVITE(const std::string &prefix,
							   const std::string &targetNick,
							   const std::string &channel)
{
	return (makeCommandReply(prefix, "INVITE", targetNick + " " + channel, ""));
}

inline std::string	RPL_KICK(const std::string &prefix,
							 const std::string &channel,
							 const std::string &target,
							 const std::string &reason)
{
	return (makeCommandReply(prefix, "KICK", channel + " " + target, reason));
}

inline std::string	RPL_MODE(const std::string &prefix,
							 const std::string &channel,
							 const std::string &modes,
							 const std::string &args)
{
	std::string	params;
	
	params = channel + " " + modes;
	if (!args.empty())
		params += " " + args;
	return (makeCommandReply(prefix, "MODE", params, ""));
}

inline std::string	RPL_NICK(const std::string &oldPrefix,
							 const std::string &newNick)
{
	return (makeCommandReply(oldPrefix, "NICK", newNick, ""));
}

/* ==================== PING / PONG ==================== */

inline std::string	RPL_PING(const std::string &serverName,
							 const std::string &token)
{
	return (makeCommandReply(serverName, "PING", "", token));
}

inline std::string	RPL_PONG(const std::string &serverName,
							 const std::string &token)
{
	return (makeCommandReply(serverName, "PONG", serverName, token));
}
}

#endif
