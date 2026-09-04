#include <Engine/Core/Application.hpp>
#include <Engine/Core/Logger.hpp>

#include <Game/States/InGame.hpp>
#include <Game/States/Menu.hpp>

int Application::Run() {
    if (!Init()) {
        Logger::Error("App Init failed!");
        return EXIT_FAILURE;
    }

    sf::Clock clock;
    while (m_window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        HandleEvents();
        Update(dt);
        Render();
    }

    if (!Destroy()) {
        Logger::Error("App Destroy failed!");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

//--- Privates
bool Application::Init() {
    m_window = sf::RenderWindow{
        sf::VideoMode{sf::Vector2u{screen_width, screen_height}},
        name, 
        sf::State::Windowed
    };

    m_assets.SetRoot("resources");
    m_assets.AcquireGroup("Common");

    //- StateManager
    m_states.Push<GameState>();
    m_states.Push<MenuState>();

    if (!m_window.isOpen()) {
        return false;
    }

    return true;
}

void Application::HandleEvents() {
    while (const auto event = m_window.pollEvent()) {

        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                m_window.close();
            }
        }
    }
}

void Application::Update(const float dt) {
    m_states.Update(dt);
}

void Application::Render() {
    m_window.clear(sf::Color{30u, 30u, 50u, 255u});

    m_states.Render(m_window);

    m_window.display();
}

bool Application::Destroy() {
    if (m_window.isOpen()) {
        m_window.close();
    }

    return true;
}