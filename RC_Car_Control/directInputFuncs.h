#pragma once
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <mmsystem.h>
#include <dinput.h>
#include <iostream>
#include <string>
#include <cmath>
#include <map>
#include "jsonParsing.h"

// Fonctions externes
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);
bool InitDirectInput(HWND hWnd);
bool testPollAcquire(HRESULT& hr);
void printData(HRESULT& hr, DIJOYSTATE& js);
void dataParsing(HRESULT& hr, DIJOYSTATE& js);
void minMaxValuesConfig(std::string configName, std::string jsElName);
void calibrationWizard();

// Variables externes
extern LPDIRECTINPUT8 g_pDI;
extern LPDIRECTINPUTDEVICE8 g_pDevice;

struct state {
    int gas_value = 0;
    int brake_value = 0;
    int gears = 0;
    int steering_value = 32768;

    int arrow_val = -1;
    bool handbrake = false;
    int left_shifter = 0;
    int right_shifter = 0;

    int left_shifter_prev = 0;
    int right_shifter_prev = 0;
};

struct settings {
    int engine_break_value = 1250;
    int engine_power = 3000;
    int brake_force = 15000;
    int max_engine_power_per_gear = 0;
};

struct toSendData {
    int steering_angle = 98;
    int throttle_value = 90;
};

extern state State;
extern settings Settings;
extern toSendData ToSendData; // remplace les valeurs atomic


