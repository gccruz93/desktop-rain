#include <windows.h>
#include "RainApp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    RainApp app(hInstance);
    return app.Run();
}
