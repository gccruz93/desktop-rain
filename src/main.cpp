#include <windows.h>
#include "RainApplication.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    RainApplication app(hInstance);
    return app.Run();
}
