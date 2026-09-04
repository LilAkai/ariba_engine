#include "StateManager.hpp"

#include <cassert>

namespace Core {

// -----------------------------------------------------------------------------
// Identité de type
// -----------------------------------------------------------------------------
namespace Detail {

TypeId NextTypeId() noexcept {
    static TypeId next = 0;
    return next++;
}

}  // namespace Detail

Context& State::GetContext() const noexcept {
    assert(m_Manager != nullptr && "GetContext() n'est valide qu'à partir de OnEnter()");
    return m_Manager->GetContext();
}

// -----------------------------------------------------------------------------
// Construction / destruction
// -----------------------------------------------------------------------------
StateManager::StateManager(Context* context, std::size_t reserve)
    : m_Context(context) {
    m_Stack.reserve(reserve);
    m_Pending.reserve(reserve);
    m_Trash.reserve(reserve);
}

StateManager::~StateManager() {
    m_Pending.clear();   // les transitions en attente n'ont plus de sens
    DoClear();           // OnExit() est appelé sur chaque état restant
    m_Trash.clear();
}

Context& StateManager::GetContext() const noexcept {
    assert(m_Context != nullptr && "aucun Context n'a été fourni au StateManager");
    return *m_Context;
}

// -----------------------------------------------------------------------------
// File de transitions
// -----------------------------------------------------------------------------
void StateManager::Enqueue(Action action, std::unique_ptr<State> state, std::size_t count) {
    m_Pending.push_back(Command{action, count, std::move(state)});
}

void StateManager::Pop(std::size_t count) {
    if (count != 0) {
        Enqueue(Action::Pop, nullptr, count);
    }
}

void StateManager::Clear() {
    Enqueue(Action::Clear, nullptr);
}

void StateManager::ApplyPending() {
    if (m_Pending.empty()) {
        return;
    }

    // OnEnter()/OnExit() peuvent enfiler de nouvelles commandes : le vector
    // grandit pendant la boucle, donc on indexe et on extrait tout dans des
    // locales avant d'appeler quoi que ce soit (une réallocation invaliderait
    // toute référence sur l'élément).
    for (std::size_t i = 0; i < m_Pending.size(); ++i) {
        const Action           action = m_Pending[i].ActionType;
        const std::size_t      count  = m_Pending[i].Count;
        std::unique_ptr<State> state  = std::move(m_Pending[i].NewState);

        switch (action) {
            case Action::Push:
                DoPush(std::move(state));
                break;
            case Action::Pop:
                DoPop(count);
                break;
            case Action::Replace:
                DoReplace(std::move(state));
                break;
            case Action::Reset:
                DoClear();
                DoPush(std::move(state));
                break;
            case Action::Clear:
                DoClear();
                break;
        }
    }

    m_Pending.clear();   // capacité conservée : plus aucune allocation ensuite
    RefreshLayers();
    m_Trash.clear();     // destruction effective, une fois toutes les transitions faites
}

// -----------------------------------------------------------------------------
// Opérations immédiates (uniquement depuis ApplyPending / le destructeur)
// -----------------------------------------------------------------------------
void StateManager::DoPush(std::unique_ptr<State> state) {
    if (!state) {
        return;
    }
    if (!m_Stack.empty()) {
        m_Stack.back()->OnPause();
    }
    m_Stack.push_back(std::move(state));
    m_Stack.back()->OnEnter();
}

void StateManager::DoPop(std::size_t count) {
    if (m_Stack.empty()) {
        return;
    }
    if (count > m_Stack.size()) {
        count = m_Stack.size();
    }

    for (std::size_t i = 0; i < count; ++i) {
        std::unique_ptr<State> state = std::move(m_Stack.back());
        m_Stack.pop_back();
        Retire(std::move(state));
    }
    if (!m_Stack.empty()) {
        m_Stack.back()->OnResume();
    }
}

void StateManager::DoReplace(std::unique_ptr<State> state) {
    // L'état du dessous reste en pause : on lui évite un OnResume() suivi d'un
    // OnPause() dans la même frame.
    if (!m_Stack.empty()) {
        std::unique_ptr<State> old = std::move(m_Stack.back());
        m_Stack.pop_back();
        Retire(std::move(old));
    }
    if (!state) {
        return;
    }
    m_Stack.push_back(std::move(state));
    m_Stack.back()->OnEnter();
}

void StateManager::DoClear() {
    while (!m_Stack.empty()) {
        std::unique_ptr<State> state = std::move(m_Stack.back());
        m_Stack.pop_back();
        Retire(std::move(state));
    }
}

void StateManager::Retire(std::unique_ptr<State> state) noexcept {
    if (!state) {
        return;
    }
    state->OnExit();
    m_Trash.push_back(std::move(state));   // détruit en fin d'ApplyPending()
}

// -----------------------------------------------------------------------------
// Couches : recalculées uniquement quand la pile change, pas à chaque frame
// -----------------------------------------------------------------------------
void StateManager::RefreshLayers() noexcept {
    m_FirstRender = 0;
    m_FirstUpdate = 0;
    if (m_Stack.empty()) {
        return;
    }

    for (std::size_t i = m_Stack.size(); i-- > 0;) {
        if (m_Stack[i]->BlocksRender) {
            m_FirstRender = i;
            break;
        }
    }
    for (std::size_t i = m_Stack.size(); i-- > 0;) {
        if (m_Stack[i]->BlocksUpdate) {
            m_FirstUpdate = i;
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Aiguillage de la frame
// -----------------------------------------------------------------------------
void StateManager::HandleEvent(const sf::Event& event) {
    for (std::size_t i = m_Stack.size(); i-- > 0;) {
        State& state = *m_Stack[i];
        state.HandleEvent(event);
        if (state.BlocksInput) {
            break;
        }
    }
}

void StateManager::FixedUpdate(const float dt) {
    for (std::size_t i = m_FirstUpdate, n = m_Stack.size(); i < n; ++i) {
        m_Stack[i]->FixedUpdate(dt);
    }
}

void StateManager::Update(const float dt) {
    ApplyPending();   // point sûr : aucune méthode d'état n'est en cours
    for (std::size_t i = m_FirstUpdate, n = m_Stack.size(); i < n; ++i) {
        m_Stack[i]->Update(dt);
    }
}

void StateManager::Render(sf::RenderTarget& target, float alpha) {
    for (std::size_t i = m_FirstRender, n = m_Stack.size(); i < n; ++i) {
        m_Stack[i]->Render(target, alpha);
    }
}

// -----------------------------------------------------------------------------
// Accès
// -----------------------------------------------------------------------------
State* StateManager::GetTop() noexcept {
    return m_Stack.empty() ? nullptr : m_Stack.back().get();
}

const State* StateManager::GetTop() const noexcept {
    return m_Stack.empty() ? nullptr : m_Stack.back().get();
}

}  // namespace Core