#pragma once
#include <dinput.h>

BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
void ListerAxes(LPDIRECTINPUTDEVICE8 device);