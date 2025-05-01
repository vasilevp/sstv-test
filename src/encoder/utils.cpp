#include "utils.hpp"

#include <atomic>
#include <print>
#include <string>

using namespace utils;

std::atomic<int> ScopeGuard::globalIndent = 0;
ScopeGuard::ScopeGuard(const std::string &function) : function(function)
{
	std::println("{}┍ Entering {}", std::string(globalIndent++, '|'), function);
}

ScopeGuard::~ScopeGuard()
{
	std::println("{}┕ Exiting {}", std::string(--globalIndent, '|'), function);
}