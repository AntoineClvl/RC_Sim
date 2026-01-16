


/* js.
rgbButtons[0..x]
lX
lY
lZ
lRx
lRy
lRz
rgdwPOV[0..x]

https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416594(v=vs.85)
*/

#include <dinput.h>
#include <iostream>
#include "setup.h"

// Callback pour EnumObjects
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
    if (pdidoi->dwType & DIDFT_AXIS)
    {
        std::wcout << L"Axe trouvé : " << pdidoi->tszName << std::endl;
    }
    return DIENUM_CONTINUE;
}

void ListerAxes(LPDIRECTINPUTDEVICE8 device)
{
    // Énumère les objets (axes, boutons, POVs, etc.)
    if (FAILED(device->EnumObjects(EnumAxesCallback, nullptr, DIDFT_AXIS)))
    {
        std::cerr << "Erreur lors de l'énumération des axes." << std::endl;
    }
}
