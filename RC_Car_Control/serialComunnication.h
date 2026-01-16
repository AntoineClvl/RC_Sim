#pragma once
#include <string>
#include <boost/asio.hpp>

void sendData(const std::string& port, unsigned int baud_rate);
std::string wcharToString(const wchar_t* wideString);
std::vector<std::string> getAvailableCOMPorts();
extern std::atomic<uint8_t> accel;
extern std::atomic<uint8_t> steer;
extern std::atomic<int> gear;
