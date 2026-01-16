#include "jsonParsing.h"

using json = nlohmann::json;

std::string brake_axis;
std::string gas_axis;
std::string clutch_axis;
std::string steering_axis;
std::string shifter_left_button;
std::string shifter_right_button;
std::string triangle_button;
std::string square_button;
std::string circle_button;
std::string cross_button;
std::string SE_button;
std::string ST_button;
std::string R2_button;
std::string L2_button;
std::string R3_button;
std::string L3_button;
std::string PS_button;
std::string arrow_button;

void initConfig() {
    if (isWheelBaseConfigNeeded()) {
        calibrationWizard();
    }
    else {
        std::ifstream inputFile("settings.json");
        if (!inputFile.is_open()) {
            std::cerr << "Impossible d'ouvrir le fichier !" << std::endl;
            return;
        }
        json settings;
        inputFile >> settings;
        inputFile.close();
        std::string configName = settings["wheelBase_ConfigName"];
        
        loadConfig(configName);
    }
}


void loadConfig(std::string configName) {
    std::ifstream file("wheelParameters.json");
    if (!file) {
        std::cerr << "Erreur d'ouverture du fichier.\n";
        return;
    }

    // check settings.json pour prendre la bonne config

    json config;
    file >> config;

    brake_axis = config["presets"][configName]["mapping"]["brake"];
    gas_axis = config["presets"][configName]["mapping"]["gas"];
    steering_axis = config["presets"][configName]["mapping"]["steering"];
    clutch_axis = config["presets"][configName]["mapping"]["clutch"];
    shifter_left_button = config["presets"][configName]["mapping"]["shifter_left"];
    shifter_right_button = config["presets"][configName]["mapping"]["shifter_right"];
    triangle_button = config["presets"][configName]["mapping"]["triangle"];
    circle_button = config["presets"][configName]["mapping"]["circle"];
    square_button = config["presets"][configName]["mapping"]["square"];
    cross_button = config["presets"][configName]["mapping"]["cross"];
    SE_button = config["presets"][configName]["mapping"]["SE"];
    ST_button = config["presets"][configName]["mapping"]["ST"];
    R2_button = config["presets"][configName]["mapping"]["R2"];
    L2_button = config["presets"][configName]["mapping"]["L2"];
    R3_button = config["presets"][configName]["mapping"]["R3"];
    L3_button = config["presets"][configName]["mapping"]["L3"];
    PS_button = config["presets"][configName]["mapping"]["PS"];
    arrow_button = config["presets"][configName]["mapping"]["arrow"];;
}

void saveConfig(std::map<std::string, std::string> toFindDic, std::string configName) {

    std::ifstream inputFile("wheelParameters.json");
    if (!inputFile.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier !" << std::endl;
        return;
    }

    json config;
    inputFile >> config;
    inputFile.close();

    for (const auto& pair : toFindDic) {
        //std::cout << pair.first << " -> " << pair.second << std::endl;
        config["presets"][configName]["mapping"][pair.first] = pair.second;
    }
    
    std::ofstream file("wheelParameters.json");
    if (!file) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier en écriture.\n";
        return;
    }
    if (file.is_open()) {
        file << config.dump(4); // dump(4) = indentation 4 espaces pour être lisible
        file.close();
    }
}

int isWheelBaseConfigNeeded() {
    std::ifstream inputFile("settings.json");
    if (!inputFile.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier !" << std::endl;
        return -1;
    }
    json settings;
    inputFile >> settings;
    inputFile.close();


    if (settings["wheelBase_isSetup"]) return 0;
    return 1;

}

void synchroCarSetupJson(std::string carSetupName, std::string elName, int value) {
    std::ifstream inputFile("carSetup.json");
    if (!inputFile.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier !" << std::endl;
        return;
    }

    json setup;
    inputFile >> setup;
    inputFile.close();

    setup["presets"][carSetupName][elName] = value;


    std::ofstream file("carSetup.json");
    if (!file) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier en écriture.\n";
        return;
    }
    if (file.is_open()) {
        file << setup.dump(4); // dump(4) = indentation 4 espaces pour être lisible
        file.close();
    }
}

void loadLimValues(std::string configName, std::string jsElName, int off, int on) {
    std::ifstream inputFile("wheelParameters.json");
    if (!inputFile.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier !" << std::endl;
        return;
    }

    json config;
    inputFile >> config;
    inputFile.close();

    config["presets"][configName]["limValues"][jsElName]["off"] = off;
    config["presets"][configName]["limValues"][jsElName]["on"]  = on;
    

    std::ofstream file("wheelParameters.json");
    if (!file) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier en écriture.\n";
        return;
    }
    if (file.is_open()) {
        file << config.dump(4); // dump(4) = indentation 4 espaces pour être lisible
        file.close();
    }
}

