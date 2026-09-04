// =============================================================================
//  Context.hpp
//  Ce que les états ont le droit de connaître du reste du programme.
//
//  Application possède les systèmes (fenêtre, assets, audio, réglages) et
//  remplit ce Context une fois, à la construction. Chaque état y accède
//  ensuite par GetContext(), sans jamais inclure ni connaître Application.
//
//      class Application {
//      public:
//          Application()
//              : m_Window(sf::VideoMode({1280, 720}), "Game")
//              , m_Context{&m_Window, &m_Assets}   // agrégat : init par accolades
//              , m_States(&m_Context) {
//              m_Assets.SetRoot("Resources");
//              m_Assets.AcquireGroup("Common");    // jamais relâché
//              m_States.Push<MenuState>();
//          }
//
//      private:
//          // ORDRE IMPORTANT : les membres sont construits dans l'ordre de
//          // déclaration, donc tout ce que pointe m_Context doit être déclaré
//          // avant lui, et m_States après lui.
//          sf::RenderWindow   m_Window;
//          Core::AssetCache   m_Assets;
//          Core::Context      m_Context;
//          Core::StateManager m_States;
//      };
//
//  Ce qui a sa place ici : ce qui est partagé ET dure autant que l'application.
//  Ce qui n'en a pas : les données d'un seul état, ou celles qui changent d'un
//  push à l'autre — ça passe par le constructeur de l'état, Push<GameState>(3).
//
//  Volontairement une struct et non une class : c'est un agrégat de pointeurs
//  non propriétaires, l'initialisation par accolades reste possible, et le
//  mot-clé correspond à la déclaration anticipée de State.hpp (MSVC émet
//  l'avertissement C4099 en cas de struct/class mélangés).
// =============================================================================
#pragma once

#include "AssetCache.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <cassert>
#include <random>

namespace Core {

struct Context {
    // --- Systèmes partagés --------------------------------------------------
    // Pointeurs NON propriétaires : Application reste le propriétaire, ce
    // Context ne fait que désigner. Ajoute tes systèmes ici au fur et à
    // mesure, aucun autre fichier n'a besoin de changer.
    sf::RenderWindow* Window = nullptr;
    AssetCache*       Assets = nullptr;

    // AudioPlayer*   Audio  = nullptr;
    // Settings*      Config = nullptr;
    // InputMapper*   Input  = nullptr;

    // --- Générateur aléatoire -----------------------------------------------
    // Possédé par le Context, lui : une seule source pour tout le jeu, ce qui
    // permet de rejouer une partie en fixant la graine.
    std::mt19937 Random{std::random_device{}()};

    void SetSeed(std::mt19937::result_type seed) { Random.seed(seed); }

    [[nodiscard]] int RandomInt(int min, int max) {
        assert(min <= max);
        return std::uniform_int_distribution<int>(min, max)(Random);
    }

    [[nodiscard]] float RandomFloat(float min, float max) {
        assert(min <= max);
        return std::uniform_real_distribution<float>(min, max)(Random);
    }

    // --- Raccourcis ---------------------------------------------------------
    // Évite d'écrire GetContext().Window->getSize() partout, et l'assert
    // signale tout de suite un Context mal rempli au lieu d'un crash opaque.
    [[nodiscard]] sf::RenderWindow& GetWindow() const {
        assert(Window != nullptr && "Context::Window n'a pas été renseigné");
        return *Window;
    }

    [[nodiscard]] AssetCache& GetAssets() const {
        assert(Assets != nullptr && "Context::Assets n'a pas été renseigné");
        return *Assets;
    }

    [[nodiscard]] sf::Vector2u GetWindowSize() const { return GetWindow().getSize(); }

    [[nodiscard]] sf::Vector2f GetWindowSizeF() const { return sf::Vector2f(GetWindowSize()); }

    [[nodiscard]] sf::Vector2f GetWindowCenter() const { return GetWindowSizeF() / 2.0F; }
};

}  // namespace Core
