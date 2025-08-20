/********************************************************************************
* @Author : hexne
* @Date   : 2025/07/24 00:52:22
********************************************************************************/
module;
#ifdef __linux__
#include <cstdio>
#include <utility>
#endif
export module command;

import std;

export class Command {
	std::string command_;
	std::string command_out_;
	explicit Command(std::string command) : command_(std::move(command));
	std::string run();
	std::string run(std::string command);
};
#ifdef _WIN32
#elif __linux__
Command::Command(std::string command) : command_(std::move(command)) {  }

std::string Command::run() {
	auto pfile = popen(command_.data(),"r");
	if (!pfile)
		throw std::runtime_error("popen() failed!");
	char buffer[512] = "";
	while(fgets(buffer,sizeof(buffer),pfile))
		command_out_ += buffer;
	fclose(pfile);
	return command_out_;
}

std::string command::run(std::string command) {
	Command cmd(std::move(command));
	return cmd.run();
}

#endif
