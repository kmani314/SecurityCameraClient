#define _GLIBCXX_USE_CXX11_ABI 0
#include "SocketException.h"

SocketException::SocketException(const char* _msg) : msg(_msg) {}

const char* SocketException::what() const noexcept {
	return msg;
}
