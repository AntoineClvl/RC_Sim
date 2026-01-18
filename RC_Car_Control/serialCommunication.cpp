#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <boost/asio.hpp>
#include <iostream> 
#include <string>
#include <fstream>
#include <cstdlib> 
#include <opencv2/opencv.hpp>               
#include <iomanip>  
#include <sstream>
#include <thread>
#include "serialComunnication.h"
#include "directInputFuncs.h"


std::atomic<uint8_t> accel(0);  
std::atomic<uint8_t> steer(0);  
std::atomic<int> gear(0);
boost::system::error_code ec;

#pragma pack(push, 1)
typedef struct Packet{
	uint8_t sof = 0xFF;  // start of frame
	uint8_t len = 8; // length 
	int32_t steering_angle;
	int32_t throttle_value;
	uint8_t eof = 0xFE;  // end of frame
    uint8_t checksum;
} Packet;
#pragma pack(pop)

void checksumCalc(Packet& P) {
	uint8_t* ptr = (uint8_t*)&P;
	uint8_t sum = 0;
	for (size_t i = 0; i < sizeof(P) - 1; ++i) {
		sum ^= ptr[i];
	}
	P.checksum = sum;
}

void sendData(const std::string& port, unsigned int baud_rate) {
    try {
        boost::asio::io_context io;
        boost::asio::serial_port serial(io, port);
        serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
        serial.set_option(boost::asio::serial_port_base::character_size(8));

        while (true) {
            if (!serial.is_open()) {
                std::cerr << "Serial port is not open. Exiting thread..." << std::endl;
                return;
            }
            Packet P;
			P.steering_angle = ToSendData.steering_angle;
			P.throttle_value = ToSendData.throttle_value;
			checksumCalc(P);
			//std::cout << ToSendData.throttle_value << " " << ToSendData.steering_angle << std::endl;   
            boost::asio::write(serial, boost::asio::buffer(&P, sizeof(Packet)));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // regler freq envoi
        }
    }
    catch (boost::system::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

std::string wcharToString(const wchar_t* wideString) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
    std::string result(bufferSize, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideString, -1, &result[0], bufferSize, NULL, NULL);
    result.pop_back(); // Supprime le caractère NULL ajouté à la fin
    return result;
}

std::vector<std::string> getAvailableCOMPorts() {
    std::vector<std::string> comPorts;
    wchar_t portName[10]; // Assez pour "COM1" à "COM256"

    for (int i = 1; i <= 256; ++i) {
        // Formater le nom du port (ex : "COM1")
        swprintf(portName, sizeof(portName) / sizeof(wchar_t), L"COM%d", i);

        // Essayer d'ouvrir le port pour voir s'il est disponible
        HANDLE hPort = CreateFile((std::wstring(L"\\\\.\\") + portName).c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPort != INVALID_HANDLE_VALUE) {
            // Ajouter directement le port COM au format "COM4" dans le vecteur
            comPorts.push_back(wcharToString(portName));
            CloseHandle(hPort);
        }
    }
    return comPorts;
}