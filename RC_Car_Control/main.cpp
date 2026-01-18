#include "directInputFuncs.h"
#include "serialComunnication.h"
#include "jsonParsing.h"
#include "setup.h"
#include "wifiTransmission.h"
#include "camera.h"
#include <thread>





/*

TODO : 
    - ajouter synchro carSetup.json avec struct settings lors du changement de paramètres avec les flèches par ex.  WIP

    - ajouter d'autres paramètres de setup pour carSetup comme les ratios de gear (plus tard)
    - ajouter des fonctionnalités aux autres boutons du volant (ouverture de paramètres, etc) (plus tard)

    - ajouter une calibration pour les valeurs limites des axes lors de calibrationWizard

    - ajouter un système d'attente de branchement du volant si aucun n'est connecté (quitte le programme actuellement)


    - boutons sur le volant peuvent actionner des choses sur l'esp32 (struct de données envoyé à l'esp ?)
*/


std::atomic<bool> running(true);

std::string port;
int baud_rate = 230400;

int main() {
    timeBeginPeriod(1);
    HRESULT hr;
    DIJOYSTATE js;

    //init steering wheel
    HWND hWnd = GetConsoleWindow();
    if (!InitDirectInput(hWnd)) {
		std::cout << "Veuillez brancher le volant..." << std::endl;
    }
    while (!InitDirectInput(hWnd)) {
        Sleep(500);
    }

    // Affiche tous les axes disponibles sur le périphérique
    /*
    if (g_pDevice) {
        std::wcout << L"Axes disponibles sur le périphérique DirectInput :" << std::endl;
        g_pDevice->EnumObjects(EnumAxesCallback, nullptr, DIDFT_ALL);
    }
    */
    


    //check com ports availables
    std::vector<std::string> availableCOMPorts = getAvailableCOMPorts();
    bool port_available = false;
    if (availableCOMPorts.empty()) std::cout << "Aucun port COM disponible." << std::endl;
    else {
        std::cout << "Ports COM disponibles :" << std::endl;
        for (const auto& port : availableCOMPorts) {
                std::cout << " - " << port << std::endl;
        }
        port = availableCOMPorts[0];
        port_available = true;
    }
    
    
    Sleep(1000);

    // Acquisition
    g_pDevice->Acquire();
    std::thread sendDataThread(sendData, port, baud_rate);
    Sleep(500);
    


    initConfig();
    
    std::thread cameraThread(cameraThreadFunc);

    while (running) { // remplacer true par port_available dans le code final
        if (!testPollAcquire(hr)) {
            Sleep(50); // attend avant retry si Poll/Acquire a échoué
            continue;
        }
        hr = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &js);
        

        //printData(hr, js); // debug
        dataParsing(hr, js);
  
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 200 Hz
        //Sleep(5);
    }

    running = false;
    if (cameraThread.joinable()) {
        cameraThread.join();
    }

    //client.cleanup();


    g_pDevice->Unacquire();
    g_pDevice->Release();
    g_pDI->Release();
    
    timeEndPeriod(1);
    
    return 0;
}






// voir chatgpt syabilisation caméra voiture... -> options pour réduire la latence avec espnow et le c++..