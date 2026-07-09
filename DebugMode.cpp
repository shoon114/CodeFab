#include "DebugMode.h"
#include <iostream>

DebugMode::DebugMode(std::string path) : path(std::move(path)) {
}

int DebugMode::Run() {
	// TODO: step/next/break/breakpoints/remove/continue/watch/unwatch/watches/inspect
	std::cout << "Debug mode is not implemented yet: " << path << std::endl;
	return 0;
}
