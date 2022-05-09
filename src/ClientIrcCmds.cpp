//
// Created by Tadeusz Kondracki on 4/2/22.
//

#include <string>
#include "Client.hpp"
#include "messages.hpp"

void Client::nick(std::vector<std::string> args) {
	if (args.size() != 1) {
		this->send_msg(ERR_NONICKNAMEGIVEN);
		return;
	}
	if (host.getClient(args[0])) {
		this->send_msg(ERR_NICKNAMEINUSE(args[0]));
		return;
	}
	if (!_user.nickname.empty()) {
		_queue.push_back(CH_NICK(_user.nickname, args[0]));
	}
	_user.nickname = args[0];
	if (!_user.username.empty() && !_user.wlc) {
		_queue.push_back(RPL_WELCOME(_user.nickname, _user.username));
		_user.wlc = true;
		_user.is_registered = true;
	}
}

void Client::userName(std::vector<std::string> args) {
	if (!_user.is_logged)
		return;
	if (_user.is_registered) {
		this->send_msg(ERR_ALREADYREGISTERED);
		return;
	}
	if (args.size() < 4) {
		this->send_msg(ERR_NEEDMOREPARAMS(this->getNickname(), "USER"));
		return;
	}
	_user.username = args[0];
	_user.mode = args[1];
	_user.unused = args[2];
	_user.real_name = args[3];
	if (!_user.nickname.empty() && !_user.wlc) {
		_queue.push_back(RPL_WELCOME(_user.nickname, _user.username));
		_user.wlc = true;
		_user.is_registered = true;
	}
}

void Client::join(std::vector<std::string> args) {
	if (args.empty()) {
		this->send_msg(ERR_NEEDMOREPARAMS(this->getNickname(), "JOIN"));
		return;
	}

	std::string password = args.size() > 1 ? args[1] : "";
	std::string channel_name = args[0];
	Channel *channel = host.getChannels(channel_name);
	if (!channel)
		channel = host.create_channel(channel_name, password, this);
	if (channel->getPassword() != password) {
		this->send_msg(ERR_BADCHANNELKEY(args[0]));
		return;
	}
	this->joinChannel(channel);
}

void Client::quit() {
	_quit = true;
	if (this->getChannel() != NULL)
		this->leaveChannel();
	std::cout << "QUIT" << std::endl;
}

void Client::pass(std::vector<std::string> args) {
	if (_user.is_registered) {
		this->send_msg(ERR_ALREADYREGISTERED);
		return;
	}

	if (args.empty()) {
		this->send_msg(ERR_NEEDMOREPARAMS(this->getNickname(), "PASS"));
		return;
	}

	if (args[0].compare(host.getPass())) {
		this->send_msg(ERR_PASSWDMISMATCH);
		return;
	}

	if (!_user.nickname.empty() && !_user.username.empty() && !_user.wlc) {
		_queue.push_back(RPL_WELCOME(_user.nickname, _user.username));
		_user.wlc = true;
		_user.is_registered = true;
	}
	_user.is_logged = true;
}

void Client::list(Client *c) {
	host.listChannel(c);
}

void	Client::part(std::vector<std::string> args) {
	if (args.empty()) {
		this->send_msg(ERR_NEEDMOREPARAMS(this->getNickname(), "PART"));
		return;
	}
	std::string	channel_name = args[0];

	Channel	*channel = host.getChannels(channel_name);
	if (!channel) {
		this->send_msg(ERR_NOSUCHCHANNEL(this->getNickname(), channel_name));
		return;
	}
	if (!this->getChannel() || this->getChannel()->getName() != channel_name) {
		this->send_msg(ERR_NOTONCHANNEL(channel_name));
		return;
	}

	this->leaveChannel();
}

void Client::pmsg(std::vector<std::string> args) {
	if (args.size() < 2 || args[0].empty() || args[0].empty()) {
		this->send_msg(ERR_NEEDMOREPARAMS(this->getNickname(), "PRIVMSG"));
		return;
	}
	std::string target = args.at(0);
	std::string message;

	for (std::vector<std::string>::iterator it = args.begin() + 1; it != args.end(); it++) {
		message.append(*it + " ");
	}

	message = message.at(0) == ':' ? message.substr(1) : message;
	message.append("\n");

	if (target.at(0) == '#') {
		Channel *channel = this->getChannel();
		if (!channel) {
			this->send_msg(ERR_NOSUCHCHANNEL(this->getNickname(), target));
			return;
		}
		channel->sendMessage(CNF_PRIVMSG(this->getPrefix(), target, message), this);
		return;
	}

	Client *dest = host.getClient(target);
	if (!dest) {
		this->send_msg(ERR_NOSUCHNICK(this->getNickname(), target));
		return;
	}
	dest->send_msg(CNF_PRIVMSG(this->getPrefix(), target, message));
}

void	Client::ping(std::vector<std::string> args)
{
	if (args.empty()){
		this->send_msg(ERR_NOORIGIN);
		return;
	}
	if (args[0] != this->host.getAddress()){
		this->send_msg(ERR_NOSUCHSERVER(args[0]));
		return;
	}
	this->send_msg("PONG " + this->host.getAddress() + "\r\n");
}

void	Client::pong(std::vector<std::string> args)
{
	if (args.empty()){
		this->send_msg(ERR_NOORIGIN);
		return;
	}
	this->set_last_activity(std::time(NULL));
}
