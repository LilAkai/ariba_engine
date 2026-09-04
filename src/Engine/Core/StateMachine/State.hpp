// =============================================================================
//  State.hpp
//  Classe de base de tous les états : Menu, Game, Pause, GameOver, Editor, ...
//
//  Dépend de SFML 3.0.2 pour ses types de données (sf::Event, sf::Time,
//  sf::RenderTarget) mais ne connaît ni la fenêtre, ni la boucle de jeu :
//  c'est Application qui les possède et qui appelle ces méthodes.
//
//      class GameState final : public Core::State {
//      public:
//          explicit GameState(int level) : m_Level(level) {}
//
//          void OnEnter() override { /* ici GetStates()/GetContext() sont valides */ }
//
//          void HandleEvent(const sf::Event& event) override {
//              if (const auto* key = event.getIf<sf::Event::KeyPressed>();
//                  key && key->code == sf::Keyboard::Key::Escape) {
//                  GetStates().Push<PauseState>();
//              }
//          }
//
//          void Update(sf::Time dt) override { m_Player.Move(dt); }
//          void Render(sf::RenderTarget& target, float) override { target.draw(m_Player); }
//
//      private:
//          int m_Level;
//      };
// =============================================================================
#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>

namespace Core {
struct Context;

class StateManager;

// -----------------------------------------------------------------------------
// Identité de type sans RTTI : un entier dense par type d'état, attribué à la
// première utilisation. Comparaison en un cycle, aucun dynamic_cast.
// -----------------------------------------------------------------------------
using TypeId = std::uint32_t;

namespace Detail {

TypeId NextTypeId() noexcept;

template <class T>
TypeId TypeIdOf() noexcept {
    static const TypeId id = NextTypeId();
    return id;
}

}  // namespace Detail

// -----------------------------------------------------------------------------
class State {
public:
    State() noexcept = default;
    virtual ~State() = default;

    State(const State&)            = delete;
    State& operator=(const State&) = delete;

    // --- Cycle de vie -------------------------------------------------------
    // OnEnter  : l'état vient d'être empilé (initialise ici, pas au constructeur).
    // OnExit   : l'état va être détruit.
    // OnPause  : un autre état a été empilé par-dessus.
    // OnResume : l'état redevient le sommet de la pile.
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnPause() {}
    virtual void OnResume() {}

    // --- Boucle -------------------------------------------------------------
    virtual void HandleEvent(const sf::Event& event) { (void)event; }
    virtual void FixedUpdate(const float dt) { (void)dt; }   // pas fixe (physique)
    virtual void Update(const float dt) { (void)dt; }        // pas variable
    virtual void Render(sf::RenderTarget& target, float alpha) {
        (void)target;
        (void)alpha;   // alpha = interpolation entre les deux derniers pas fixes
    }

    // --- Transparence -------------------------------------------------------
    // Simples données, pas d'appels virtuels : le manager en déduit une fois
    // pour toutes quelles couches traiter quand la pile change.
    //   BlocksRender = false -> l'état du dessous est dessiné aussi (menu pause)
    //   BlocksUpdate = false -> l'état du dessous continue de tourner
    //   BlocksInput  = false -> l'évènement redescend d'un cran
    bool BlocksRender = true;
    bool BlocksUpdate = true;
    bool BlocksInput  = true;

    // --- Accès --------------------------------------------------------------
    // Valides à partir de OnEnter(), jamais dans le constructeur.
    [[nodiscard]] StateManager& GetStates() const noexcept { return *m_Manager; }
    [[nodiscard]] Context&      GetContext() const noexcept;
    [[nodiscard]] TypeId        GetTypeId() const noexcept { return m_TypeId; }

private:
    friend class StateManager;

    StateManager* m_Manager = nullptr;
    TypeId        m_TypeId  = 0;
};

}  // namespace Core