// src/main/main.cpp
#include <iostream>
#include "core/Application.h"

int main() {
    engine::Application app(1200, 800, "Sim");
    app.Run();
    
    return 0;
}