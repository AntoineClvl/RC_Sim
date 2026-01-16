#pragma once
#include "json.hpp"
#include "directInputFuncs.h"
#include "serialComunnication.h"
#include "directInputFuncs.h"
#include <fstream>
#include <string>

void loadConfig(std::string configName);
void initConfig();
void saveConfig(std::map<std::string, std::string> toFindDic, std::string configName);
int isWheelBaseConfigNeeded();
void synchroCarSetupJson(std::string carSetupName, std::string elName, int value);
void loadLimValues(std::string configName, std::string jsElName, int off, int on);

extern std::string brake_axis;
extern std::string gas_axis;
extern std::string clutch_axis;
extern std::string steering_axis;
extern std::string shifter_left_button;
extern std::string shifter_right_button;
extern std::string triangle_button;
extern std::string square_button;
extern std::string circle_button;
extern std::string cross_button;
extern std::string SE_button;
extern std::string ST_button;
extern std::string R2_button;
extern std::string L2_button;
extern std::string R3_button;
extern std::string L3_button;
extern std::string PS_button;
extern std::string arrow_button;