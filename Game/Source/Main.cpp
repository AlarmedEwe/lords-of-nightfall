#include "App.h"
#include <iostream>

int main()
{
    App app;
    if (!app.Init())
    {
        std::cerr << "Initialization failed. Exiting." << std::endl;
        return 1;
    }
    app.Run();
    app.Shutdown();
    return 0;
}
