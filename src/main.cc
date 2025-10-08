// src/main/main.cpp
#include <iostream>
#include "core/Application.h"

int main() {
    engine::Application app(1920, 1080, "Sim");
    app.Run();
    
    return 0;
}