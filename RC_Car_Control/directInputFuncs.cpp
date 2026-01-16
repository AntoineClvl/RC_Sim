#include "directInputFuncs.h"
#include "serialComunnication.h"

constexpr double BRAKE_CONST = 7.0274855e-5;

constexpr float GEAR_RATIO_R = -0.25f;
constexpr float GEAR_RATIO_N = 0.0f;
constexpr float GEAR_RATIO_1 = 0.2f;
constexpr float GEAR_RATIO_2 = 0.22f;
constexpr float GEAR_RATIO_3 = 0.25f;
constexpr float GEAR_RATIO_4 = 0.3f;
constexpr float GEAR_RATIO_5 = 1.0f;

constexpr int MOD_BRAKE_FORCE = 250;
constexpr int MOD_ENGINE_POWER = 100;

constexpr int STEERING_IN_MIN = 21000; // 32768 - x
constexpr int STEERING_IN_MAX = 44536; // 32768 + x
constexpr int STEERING_OUT_MIN = 0;
constexpr int STEERING_OUT_MAX = 65535;

constexpr int MAPPED_GAS_IN_MIN = -65536;
constexpr int MAPPED_GAS_IN_MAX = 65535;
constexpr int MAPPED_GAS_OUT_MIN = 0;
constexpr int MAPPED_GAS_OUT_MAX = 180;

constexpr int MAPPED_STEERING_IN_MIN = 0;
constexpr int MAPPED_STEERING_IN_MAX = 65535;
constexpr int MAPPED_STEERING_OUT_MIN = 70; // valeurs expérimentales du servo
constexpr int MAPPED_STEERING_OUT_MAX = 126;

// Joystick Pointers
LPDIRECTINPUT8 g_pDI = nullptr;
LPDIRECTINPUTDEVICE8 g_pDevice = nullptr;

struct JoystickStateMap {
    DIJOYSTATE* js; // Pointeur vers l'état actuel du joystick

    // Une seule map qui contient soit un LONG*, soit un BYTE*
    std::map<std::string, std::variant<LONG*, BYTE*, DWORD*>> input_map;

    JoystickStateMap(DIJOYSTATE* joystickState) : js(joystickState) {
        // Axes
        input_map["lX"] = &js->lX;
        input_map["lY"] = &js->lY;
        input_map["lZ"] = &js->lZ;
        input_map["lRx"] = &js->lRx;
        input_map["lRy"] = &js->lRy;
        input_map["lRz"] = &js->lRz;
        input_map["rgdwPOV[0]"] = &js->rgdwPOV[0];

        // Boutons
        for (int i = 0; i < 12; ++i) {
            input_map["rgbButtons[" + std::to_string(i) + "]"] = &js->rgbButtons[i];
        }

        // Sliders
        for (int i = 0; i < 3; ++i) {
            input_map["rglSlider[" + std::to_string(i) + "]"] = &js->rglSlider[i];
        }
    }

    // Accès générique : renvoie toujours un LONG pour simplifier
    LONG getValue(const std::string& name) const {
        auto it = input_map.find(name);
        if (it == input_map.end()) return 0;

        if (std::holds_alternative<LONG*>(it->second)) {
            return *std::get<LONG*>(it->second);
        }
        else if (std::holds_alternative<BYTE*>(it->second)) {
            return *std::get<BYTE*>(it->second);
        }
        else if (std::holds_alternative<DWORD*>(it->second)) {
            return static_cast<LONG>(*std::get<DWORD*>(it->second));
        }
        return 0;
    }
};



// Mode
bool logitech = false;
bool thrustmaster = true;


state State;
settings Settings;
toSendData ToSendData;

bool rising_edge = false;

int mapped_gas_value = 90;
int mapped_steering_value = 97; // valeurs pour l'esp au repos


int mapFunc(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Callback : sélectionne le premier périphérique trouvé
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    HRESULT hr = g_pDI->CreateDevice(pdidInstance->guidInstance, &g_pDevice, NULL);
    return FAILED(hr) ? DIENUM_CONTINUE : DIENUM_STOP;
}

// Fonction d'initialisation de DirectInput
bool InitDirectInput(HWND hWnd) {
    HRESULT hr;

    // Création de l'interface DirectInput
    hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&g_pDI, NULL);
    if (FAILED(hr)) {
        std::cerr << "[Erreur] DirectInput8Create a échoué. Code: 0x" << std::hex << hr << std::endl;
        return false;
    }

    // Recherche du périphérique de jeu
    hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
    if (!g_pDevice) {
        std::cerr << "[Erreur] Aucun périphérique de jeu détecté." << std::endl;
        return false;
    }

    // Définition du format des données (axes, boutons, POV, sliders, etc.)
    hr = g_pDevice->SetDataFormat(&c_dfDIJoystick);
    if (FAILED(hr)) {
        std::cerr << "[Erreur] SetDataFormat a échoué. Code: 0x" << std::hex << hr << std::endl;
        return false;
    }

    // Définition du mode de coopération (arrière-plan et non exclusif pour éviter le blocage des autres périphériques)
    hr = g_pDevice->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr)) {
        std::cerr << "[Erreur] SetCooperativeLevel a échoué. Code: 0x" << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool testPollAcquire(HRESULT &hr) {
    hr = g_pDevice->Poll();
    if (FAILED(hr)) {
        //std::cout << "Poll() a échoué, tentative Acquire()..." << std::endl;
        hr = g_pDevice->Acquire();

        if (FAILED(hr)) {
            //std::cout << "Acquire() a aussi échoué : code = " << std::hex << hr << std::endl;
            Sleep(100);
            return false;
        }

        //std::cout << "Acquire réussi." << std::endl;
        return false;
    }
    return true;
}

void printData(HRESULT &hr, DIJOYSTATE &js) { // debug function
    std::string arrow;
     // x : volant , rz : accel , y : frein

    if (SUCCEEDED(hr)) {
        if (js.rgdwPOV[0] == -1)
            arrow = "Centered";
        else{
            int arrow_val = (js.rgdwPOV[0] / 100);
            switch (arrow_val) {
                case 0:
                    arrow = "Up";
                    break;
                case 90:
                    arrow = "Right";
                    break;
                case 180:
                    arrow = "Down";
                    break;
                case 270:
                    arrow = "Left";
                    break;
                case 45:
                    arrow = "Up/Right";
                    break;
                case 135:
                    arrow = "Down/Right";
                    break;
                case 225:
                    arrow = "Down/Left";
                    break;
                case 315:
                    arrow = "Up/Left";
                    break;
                default:
                    arrow = "Unknown";
                    break;
            }
       }
        int left_shifter = js.rgbButtons[0] ? 1 : 0;
        int right_shifter = js.rgbButtons[1] ? 1 : 0;
 
        std::cout << "Gas Pedal : " << js.lRz << ", Brake Pedal : " << js.lY << ", Steering angle : " << js.lX << ", Arrows : " << arrow << ", Shifter L : " << left_shifter << ", Shifter R : " << right_shifter  << "       " << "\r";
        //std::cout << "test" << "\r";
        //std::cout << "\033[1A"; // moves one line up (all of this makes the console to stop spamming printing messages)
    }
}


// Arrows mode -> change brake and gas forces, maybe add completely differents modes later

void arrowUp() { // up/down : adjust brake 
    Settings.brake_force += MOD_BRAKE_FORCE;
    if (Settings.brake_force > 20000)Settings.brake_force = 20000;
    std::cout << "Brake force : +" << MOD_BRAKE_FORCE << ", Total : " << Settings.brake_force << "                    " << std::endl;
}
void arrowDown() {
    Settings.brake_force -= MOD_BRAKE_FORCE;
    if (Settings.brake_force < 0)Settings.brake_force = 0;
    std::cout << "Brake force : -" << MOD_BRAKE_FORCE << ", Total : " << Settings.brake_force << "                    " << std::endl;
}
void arrowRight() { // right/left : adjust gas force
    Settings.engine_power += MOD_ENGINE_POWER;
    if (Settings.engine_power > 4000)Settings.engine_power = 4000;
    std::cout << "Gas force : +" << MOD_ENGINE_POWER << ", Total : " << Settings.engine_power << "                    " << std::endl;
}
void arrowLeft() {
    Settings.engine_power -= MOD_ENGINE_POWER;
    if (Settings.engine_power < 0)Settings.engine_power = 0;
    std::cout << "Gas force : -" << MOD_ENGINE_POWER << ", Total : " << Settings.engine_power << "                    " << std::endl;
}

void arrowManagement(DIJOYSTATE &js) {
    JoystickStateMap map(&js);
    State.arrow_val = (map.getValue(arrow_button) == -1) ? (-1) : ((map.getValue(arrow_button) / 100));
    
    if (!rising_edge && State.arrow_val != -1) {
        rising_edge = true;
        switch (State.arrow_val) {
            case 0 :
                arrowUp();
                break;
            case 90:
                arrowRight();
                break;
            case 180:
                arrowDown();
                break;
            case 270:
                arrowLeft();
                break;
        }
    }
    if (State.arrow_val == -1) {
        rising_edge = false;
    }
}

void brake_gasManagement(DIJOYSTATE& js) {
    JoystickStateMap map(&js);
    int raw_brake = -(map.getValue(brake_axis) - 65535);
    int raw_gas = -(map.getValue(gas_axis) - 65535);
    State.gas_value = raw_gas;
    // courbe frein
    float brake_force = (float)(std::exp(BRAKE_CONST * raw_brake) / 100.0f) * Settings.brake_force;
    State.brake_value = (raw_brake > 0) ? (int)round(brake_force) : 0;

    // gaz brut transformé en puissance moteur
    int engine_force = mapFunc(raw_gas, 0, 65535, 0, Settings.engine_power);

    //frein
    if (State.brake_value > 0 && State.gears > -1) {
        State.gas_value -= State.brake_value; 
    }
    else if (State.brake_value > 0 && State.gears == -1) {
        State.gas_value += State.brake_value;
    }
    /*
    * // acceleration
    else if (State.gears == -1) {
        State.gas_value -= engine_force;
    }
    else { // A FAIRE : donner une limite selon position de pedale et vitesse enclenchée 
        State.gas_value += engine_force; // accélération
    }

    // frein moteur 
    if (State.gas_value > 0 && raw_gas == 0) {
        State.gas_value -= Settings.engine_break_value;
    }
    else if (State.gas_value < 0 && raw_gas == 0) {
        State.gas_value += Settings.engine_break_value;
    }
    */
    if (State.gears == -1) State.gas_value = -State.gas_value; // ....
    // clamp
    if (State.gears == -1 && State.gas_value > 0)State.gas_value = 0;
    if (State.gears > -1 && State.gas_value < 0) State.gas_value = 0;
    if (State.gas_value > 65535) State.gas_value = 65535; // peut etre inutile
    if (State.gears > -1 && State.gas_value > Settings.max_engine_power_per_gear) State.gas_value = Settings.max_engine_power_per_gear;
    if (State.gears == -1 && State.gas_value < Settings.max_engine_power_per_gear) State.gas_value = Settings.max_engine_power_per_gear;
    
}

void shifterManagement(DIJOYSTATE& js) {
    JoystickStateMap map(&js);
    State.left_shifter = map.getValue(shifter_left_button) ? 1 : 0;
    State.right_shifter = map.getValue(shifter_right_button) ? 1 : 0;

    //rising edge shifter
    bool left_shifter_on = (State.left_shifter == 1 && State.left_shifter_prev == 0);
    bool right_shifter_on = (State.right_shifter == 1 && State.right_shifter_prev == 0);
    State.left_shifter_prev = State.left_shifter;
    State.right_shifter_prev = State.right_shifter;

    // shifting
    if (left_shifter_on && State.gears > -1) {
        (State.gears)--; // donwshifting
    }
    if (right_shifter_on && State.gears < 5) {
        (State.gears)++; // upshifting
    }

    switch (State.gears) {
    case (-1): // reverse 
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_R);
        break;
    case 0:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_N); // modifier pour juste faire un frein moteur
        break;
    case 1:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_1);
        break;
    case 2:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_2);
        break;
    case 3:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_3);
        break;
    case 4:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_4);
        break;
    case 5:
        Settings.max_engine_power_per_gear = (int)((float)65535 * GEAR_RATIO_5);
        break;
    default:
        Settings.max_engine_power_per_gear = 0;
        break;
    }
}

void handbrakeManagement(DIJOYSTATE& js) {
    JoystickStateMap map(&js);
    State.handbrake = ((int)map.getValue(R2_button) > 0) ? true : false;
    if (State.handbrake) State.gas_value = 0;
}

void espDataManagement() {
    //int gas_out = (State.gears < 0) ? -State.gas_value : State.gas_value;


    mapped_gas_value = mapFunc(State.gas_value, MAPPED_GAS_IN_MIN, MAPPED_GAS_IN_MAX, MAPPED_GAS_OUT_MIN, MAPPED_GAS_OUT_MAX); // voir librairie servo et mettre en microsecondes
    mapped_steering_value = mapFunc(State.steering_value, MAPPED_STEERING_IN_MIN, MAPPED_STEERING_IN_MAX, MAPPED_STEERING_OUT_MIN, MAPPED_STEERING_OUT_MAX);

    ToSendData.throttle_value = mapped_gas_value;
    ToSendData.steering_angle = mapped_steering_value;
    accel.store(mapped_gas_value);
    steer.store(mapped_steering_value);
    gear.store(State.gears);
}

void steeringManagement(DIJOYSTATE& js) {
    JoystickStateMap map(&js);
    State.steering_value = map.getValue(steering_axis);

    //set steering limits
    if (State.steering_value > STEERING_IN_MAX) State.steering_value = STEERING_IN_MAX;
    if (State.steering_value < STEERING_IN_MIN) State.steering_value = STEERING_IN_MIN;
    State.steering_value = mapFunc(State.steering_value, STEERING_IN_MIN, STEERING_IN_MAX, STEERING_OUT_MIN, STEERING_OUT_MAX);
}

void dataParsing(HRESULT& hr, DIJOYSTATE& js) { // parsing data in order to send it with functions from serialCommunication.cpp
    if (SUCCEEDED(hr)) {

        if (thrustmaster) {

            arrowManagement(js);
            shifterManagement(js);
            steeringManagement(js);
            brake_gasManagement(js);
            handbrakeManagement(js);
            espDataManagement();

            
            //std::cout << map.getValue(brake_axis) << std::endl;
            std::cout << "gas_mapped  : " << ToSendData.throttle_value << ", steering_mapped : " << ToSendData.steering_angle << "              \r" << std::flush;
            //std::cout << "Gas : " << State.gas_value << ", Brake : " << State.brake_value << ", Gears : " << State.gears << ", Steering : " << State.steering_value << "              \r" << std::flush; // debug
        }
        
    }
}

void calibrationWizard() {
    HRESULT hr, hrInit;
    DIJOYSTATE js, jsInit;

    std::string axes_sliders_button[50]{
        "lX",
        "lY",
        "lZ",
        "lRx",
        "lRy",
        "lRz",
        "rglSlider[0]",
        "rglSlider[1]"
    };

    for (int i = 8; i < 29; i++) {
        axes_sliders_button[i] = "rgbButtons[" + std::to_string(i-8) + "]";
    }

    for (int i = 29; i < 33; i++) {
        axes_sliders_button[i] = "rgdwPOV[" + std::to_string(i - 29) + "]";
    }

    std::map<std::string, std::string> toFindDic;
    std::string toFind[18]{
        "gas",
        "brake",
        "clutch",
        "steering",
        "shifter_left",
        "shifter_right",
        "L2",
        "R2",
        "L3",
        "R3",
        "cross",
        "circle",
        "square",
        "triangle",
        "arrow",
        "SE",
        "ST",
        "PS"
    };


    std::string var = "";

    // double actualisation pour éviter les bugs lors du chargement de la valeur initiale (peut etre pas ce probleme avec DIJOYSTATE2)
    testPollAcquire(hrInit);
    hrInit = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &jsInit);
    Sleep(200);
    testPollAcquire(hrInit);
    hrInit = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &jsInit);
    Sleep(1000);

    for (int j = 0; j < 18; j++) {
        std::cout << "Calibration de : " << toFind[j] << std::endl;

        
        //testPollAcquire(hrInit);
        //hrInit = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &jsInit);
        Sleep(200);
        testPollAcquire(hrInit);
        hrInit = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &jsInit);

        std::cout << "Appuyer sur : " << toFind[j] << std::endl;

        bool found = false;
        while (!found) {
            if (!testPollAcquire(hr)) {
                Sleep(50); // attend avant retry si Poll/Acquire a échoué
                continue;
            }
            hr = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &js);

            //DIJOYSTATE2 a plus d'axes et bouttons, voir pour changer le code entier en DIJOYSTATE2 si besoin
            LONG diffs[8] = {
                abs(js.lX - jsInit.lX),
                abs(js.lY - jsInit.lY),
                abs(js.lZ - jsInit.lZ),
                abs(js.lRx - jsInit.lRx),
                abs(js.lRy - jsInit.lRy),
                abs(js.lRz - jsInit.lRz),
                abs(js.rglSlider[0] - jsInit.rglSlider[0]),
                abs(js.rglSlider[1] - jsInit.rglSlider[1])
            };


            for (int i = 0; i < 8; i++) {
                if (diffs[i] > 2000) {
                    std::cout << toFind[j] << " trouvé" << std::endl;
                    
                    std::cout << "Axe : " << axes_sliders_button[i] << std::endl;
                    var = axes_sliders_button[i];
                    
                    found = true;
                    break;

                }
            }

            for (int i = 0; i < 20; i++) {
                if (js.rgbButtons[i] != jsInit.rgbButtons[i]) {
                    std::cout << toFind[j] << " trouvé" << std::endl;
                    
                    std::cout << "Axe : " << axes_sliders_button[8+i] << std::endl;
                    var = axes_sliders_button[8 + i];

                    found = true;
                    break;
                }
            }

            for (int i = 0; i < 4; i++) {
                if (js.rgdwPOV[i] != jsInit.rgdwPOV[i]) {
                    std::cout << toFind[j] << " trouvé" << std::endl;
                    
                    std::cout << "Axe : " << axes_sliders_button[29 + i] << std::endl;
                    var = axes_sliders_button[29 + i];

                    found = true;
                    break;
                }
            }
        }
        toFindDic[toFind[j]] = var;
        Sleep(1000);
    }

    std::string configName;
    std::cout << "Donnez un nom à la config : " << std::endl;
    std::cin >> configName;

    for (int i = 0; i < 18; i++) {
        minMaxValuesConfig(configName, toFindDic[toFind[i]]);
    }

    saveConfig(toFindDic, configName);
    loadConfig(configName);
}

void minMaxValuesConfig(std::string configName, std::string jsElName) {
    HRESULT hr;
    DIJOYSTATE js;
    JoystickStateMap map(&js);

    std::cout << "Relachez au maximum : " << jsElName << std::endl;
    Sleep(500);

    auto start = std::chrono::high_resolution_clock::now();
    
    int off, on = 0;
    int prev_value = map.getValue(jsElName);
    int current_value = map.getValue(jsElName);
    bool offValueDetected = false;
    bool onValueDetected = false;

    while (!offValueDetected) {
        if (!testPollAcquire(hr)) {
            Sleep(50); // attend avant retry si Poll/Acquire a échoué
            continue;
        }
        hr = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &js);
        current_value = map.getValue(jsElName);

        auto end = std::chrono::high_resolution_clock::now();

        if (current_value != prev_value) {
            start = std::chrono::high_resolution_clock::now();
        }
        if (current_value == prev_value && (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count() > 2000) {
            off = current_value;
            offValueDetected = true;
        }
        prev_value = current_value;
    }
    std::cout << "Valeur off : " << off << std::endl;
    std::cout << "Appuyez au maximum" << std::endl;

    start = std::chrono::high_resolution_clock::now();

    while (!onValueDetected) {
        if (!testPollAcquire(hr)) {
            Sleep(50); // attend avant retry si Poll/Acquire a échoué
            continue;
        }
        hr = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE), &js);
        current_value = map.getValue(jsElName);

        auto end = std::chrono::high_resolution_clock::now();

        if (current_value != prev_value) {
            start = std::chrono::high_resolution_clock::now();
        }
        if (current_value == prev_value && (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count() > 2000) {
            on = current_value;
            onValueDetected = true;
        }
        prev_value = current_value;
    }
    std::cout << "Valeur on : " << on << std::endl;
    
    loadLimValues(configName, jsElName, off, on);

    // Possible probleme si les valeurs son inversées, refaire les calculs si possible pour que ça marche pour tout type de valeur
    // mettre les valeurs limites dans wheelParameters.json et au loadConfig(), load ces valeurs dans les constexpr en haut
}