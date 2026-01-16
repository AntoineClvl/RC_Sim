#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include "directInputFuncs.h"
#include "jsonParsing.h"

#pragma comment(lib, "ws2_32.lib") // lib Winsock

class UDPClientESP32 {
public:
    UDPClientESP32(const char* ip, uint16_t port);
    ~UDPClientESP32();

    // Initialise Winsock et crée la socket
    bool init();

    // Envoie une struct MyData
    bool sendData(const toSendData& data);

    // Nettoyage (closesocket + WSACleanup)
    void cleanup();

private:
    const char* m_ip;
    uint16_t m_port;
    SOCKET m_sock;
    sockaddr_in m_addr;
    bool m_initialized;
};


