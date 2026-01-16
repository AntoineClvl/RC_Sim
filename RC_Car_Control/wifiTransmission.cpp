#include "wifiTransmission.h"
#include <iostream>

UDPClientESP32::UDPClientESP32(const char* ip, uint16_t port)
    : m_ip(ip), m_port(port), m_sock(INVALID_SOCKET), m_initialized(false) {
    ZeroMemory(&m_addr, sizeof(m_addr));
}

UDPClientESP32::~UDPClientESP32() {
    cleanup();
}

bool UDPClientESP32::init() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip, &m_addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address\n";
        closesocket(m_sock);
        WSACleanup();
        return false;
    }

    m_initialized = true;
    return true;
}

bool UDPClientESP32::sendData(const toSendData& data) {
    if (!m_initialized) return false;

    int s = sendto(m_sock, reinterpret_cast<const char*>(&data), sizeof(data), 0,
        reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr));
    if (s == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;
}

void UDPClientESP32::cleanup() {
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
    if (m_initialized) {
        WSACleanup();
        m_initialized = false;
    }
}




/*
int main() {
    UDPClientESP32 client("192.168.4.1", 3333);

    if (!client.init()) {
        std::cerr << "Failed to initialize UDP client\n";
        return 1;
    }

    MyData data{1, 2};

    while (true) {
        client.sendData(data);
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 200 Hz
    }

    client.cleanup();
    return 0;
}
*/




