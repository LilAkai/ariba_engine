#pragma once 
#include <string>
#include <string_view>

#include <SFML/Graphics/RenderWindow.hpp>

#include <Engine/Core/StateMachine/StateManager.hpp>
#include <Engine/Core/Context.hpp>
#include <Engine/Core/AssetCache.hpp>

class Application final {
public:
    Application(const std::string& name, unsigned int width = 1280u, 
                unsigned int height = 720u, unsigned int fps = 60u):
    name(name), screen_width(width), screen_height(height), fps_cap(fps),
    m_states(&m_context), m_context(&m_window, &m_assets) {}

    int Run();

private:
    bool Init();
    void HandleEvents();
    void Update(const float dt);
    void Render();
    bool Destroy();        

    unsigned int screen_width;
    unsigned int screen_height;
    unsigned int fps_cap;

    std::string name;

    sf::RenderWindow   m_window;
    Core::AssetCache   m_assets;
    Core::Context      m_context;
    Core::StateManager m_states; 
};