// =============================================================================
//  StateManager.hpp
//  Pile d'états, et rien d'autre : pas de fenêtre, pas d'horloge, pas de boucle.
//  Il empile, dépile et aiguille les appels vers les bons états. C'est ton
//  Application qui possède la sf::RenderWindow et qui pilote la frame.
//
//      class Application {
//      public:
//          Application()
//              : m_Window(sf::VideoMode({1280, 720}), "Game")
//              , m_Context{&m_Window, &m_Assets}
//              , m_States(&m_Context) {
//              m_States.Push<MenuState>();
//          }
//
//          void Run() {
//              sf::Clock clock;
//              while (m_Window.isOpen() && m_States.IsRunning()) {
//                  const sf::Time dt = clock.restart();
//
//                  while (const std::optional event = m_Window.pollEvent()) {
//                      if (event->is<sf::Event::Closed>()) { m_Window.close(); }
//                      m_States.HandleEvent(*event);
//                  }
//
//                  m_States.Update(dt);
//
//                  m_Window.clear();
//                  m_States.Render(m_Window);
//                  m_Window.display();
//              }
//          }
//
//      private:
//          sf::RenderWindow   m_Window;
//          AssetsCache        m_Assets;
//          Core::Context      m_Context;
//          Core::StateManager m_States;
//      };
//
//  Garanties :
//   - Un état peut appeler GetStates().Pop() / Push<T>() depuis n'importe
//     laquelle de ses méthodes : la transition est appliquée au prochain
//     point sûr.
//   - Un état détruit ne l'est jamais pendant l'exécution d'une de ses méthodes.
//   - Aucune allocation par frame (les tampons gardent leur capacité).
//   - Aucun RTTI, aucun dynamic_cast, aucune std::function.
// =============================================================================
#pragma once

#include "State.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Core {

class StateManager final {
public:
    // context peut rester nullptr si tes états n'ont rien à partager.
    explicit StateManager(Context* context = nullptr, std::size_t reserve = 8);
    ~StateManager();

    StateManager(const StateManager&)            = delete;
    StateManager& operator=(const StateManager&) = delete;

    // --- Transitions --------------------------------------------------------
    // L'état est construit tout de suite (avec tes arguments), mais inséré dans
    // la pile au prochain ApplyPending() : construire n'est jamais dangereux,
    // détruire l'est.
    template <class T, class... TArgs> void Push(TArgs&&... args);
    template <class T, class... TArgs> void Replace(TArgs&&... args);   // remplace le sommet
    template <class T, class... TArgs> void Reset(TArgs&&... args);     // vide tout puis empile
    void Pop(std::size_t count = 1);
    void Clear();

    [[nodiscard]] bool HasPendingTransition() const noexcept { return !m_Pending.empty(); }
    void ApplyPending();   // appelé automatiquement au début de Update()

    // --- Aiguillage de la frame ---------------------------------------------
    // À appeler depuis ta boucle. FixedUpdate est optionnel : ne l'appelle que
    // si tu tiens un accumulateur de pas fixe.
    void HandleEvent(const sf::Event& event);
    void FixedUpdate(const float dt);
    void Update(const float dt);
    void Render(sf::RenderTarget& target, float alpha = 1.0F);

    // --- Accès --------------------------------------------------------------
    // Condition de boucle à utiliser : contrairement à IsEmpty(), tient compte
    // des transitions en attente (au premier tour, la pile est encore vide).
    [[nodiscard]] bool         IsRunning() const noexcept { return !m_Stack.empty() || !m_Pending.empty(); }
    [[nodiscard]] bool         IsEmpty() const noexcept { return m_Stack.empty(); }
    [[nodiscard]] std::size_t  GetSize() const noexcept { return m_Stack.size(); }
    [[nodiscard]] State*       GetTop() noexcept;
    [[nodiscard]] const State* GetTop() const noexcept;
    [[nodiscard]] Context&     GetContext() const noexcept;

    // Recherche du sommet vers le bas ; nullptr si absent. Comparaison d'entiers.
    template <class T> [[nodiscard]] T*   Find() const noexcept;
    template <class T> [[nodiscard]] bool Has() const noexcept { return Find<T>() != nullptr; }

private:
    enum class Action : std::uint8_t {
        Push,
        Pop,
        Replace,
        Reset,
        Clear
    };

    struct Command {
        Action                 ActionType;
        std::size_t            Count;      // pour Pop
        std::unique_ptr<State> NewState;   // pour Push / Replace / Reset
    };

    template <class T, class... TArgs> std::unique_ptr<State> Make(TArgs&&... args);
    void Enqueue(Action action, std::unique_ptr<State> state, std::size_t count = 0);

    void DoPush(std::unique_ptr<State> state);
    void DoPop(std::size_t count);
    void DoReplace(std::unique_ptr<State> state);
    void DoClear();
    void Retire(std::unique_ptr<State> state) noexcept;   // OnExit() + mise au rebut
    void RefreshLayers() noexcept;

    Context* m_Context;

    std::vector<std::unique_ptr<State>> m_Stack;
    std::vector<Command>                m_Pending;
    std::vector<std::unique_ptr<State>> m_Trash;   // destruction repoussée en fin de transition

    std::size_t m_FirstRender = 0;   // 1re couche à dessiner
    std::size_t m_FirstUpdate = 0;   // 1re couche à mettre à jour
};

// =============================================================================
//  Implémentation des templates
// =============================================================================
template <class T, class... TArgs>
std::unique_ptr<State> StateManager::Make(TArgs&&... args) {
    static_assert(std::is_base_of_v<State, T>, "T doit dériver de Core::State");

    std::unique_ptr<State> state(new T(std::forward<TArgs>(args)...));
    state->m_Manager = this;
    state->m_TypeId  = Detail::TypeIdOf<T>();
    return state;
}

template <class T, class... TArgs>
void StateManager::Push(TArgs&&... args) {
    Enqueue(Action::Push, Make<T>(std::forward<TArgs>(args)...));
}

template <class T, class... TArgs>
void StateManager::Replace(TArgs&&... args) {
    Enqueue(Action::Replace, Make<T>(std::forward<TArgs>(args)...));
}

template <class T, class... TArgs>
void StateManager::Reset(TArgs&&... args) {
    Enqueue(Action::Reset, Make<T>(std::forward<TArgs>(args)...));
}

template <class T>
T* StateManager::Find() const noexcept {
    const TypeId wanted = Detail::TypeIdOf<T>();
    for (std::size_t i = m_Stack.size(); i-- > 0;) {
        if (m_Stack[i]->m_TypeId == wanted) {
            return static_cast<T*>(m_Stack[i].get());
        }
    }
    return nullptr;
}

}  // namespace Core