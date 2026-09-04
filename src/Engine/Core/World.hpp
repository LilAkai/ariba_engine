#pragma once
#include <Engine/Core/Components/ComponentManager.hpp>
#include <Engine/Core/Entities/Entity.hpp>
#include <Engine/Core/Entities/EntityManager.hpp>
#include <Engine/Core/Systems/System.hpp>
#include <Engine/Core/Systems/SystemManager.hpp>
#include "View.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

class World {
public:
    World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // --- Entités ---

    Entity CreateEntity() { return m_Entities.Add(); }

    void DestroyEntity(const Entity& Value) {
        if (!m_Entities.IsAlive(Value)) {
            return;
        }

        m_Components.RemoveAll(Value);
        m_Entities.Remove(Value);
    }

    bool IsAlive(const Entity& Value) const { return m_Entities.IsAlive(Value); }

    std::size_t GetEntityCount() const { return m_Entities.GetCount(); }

    const std::vector<Entity>& GetEntities() const { return m_Entities.GetEntities(); }

    // --- Composants ---

    template<typename TComponent, typename... TArgs>
    TComponent& AddComponent(const Entity& Value, TArgs&&... Args) {
        if (!m_Entities.IsAlive(Value)) {
            throw std::invalid_argument("AddComponent : entite morte ou invalide.");
        }

        return m_Components.GetOrCreateStorage<TComponent>()
            .Emplace(Value, std::forward<TArgs>(Args)...);
    }

    template<typename TComponent>
    void RemoveComponent(const Entity& Value) {
        if (ComponentStorage<TComponent>* Storage = m_Components.FindStorage<TComponent>()) {
            Storage->Remove(Value);
        }
    }

    template<typename TComponent>
    bool HasComponent(const Entity& Value) const {
        const ComponentStorage<TComponent>* Storage = m_Components.FindStorage<TComponent>();
        return Storage != nullptr && Storage->Has(Value);
    }

    template<typename TComponent>
    TComponent* TryGetComponent(const Entity& Value) {
        ComponentStorage<TComponent>* Storage = m_Components.FindStorage<TComponent>();
        return Storage == nullptr ? nullptr : Storage->TryGet(Value);
    }

    template<typename TComponent>
    const TComponent* TryGetComponent(const Entity& Value) const {
        const ComponentStorage<TComponent>* Storage = m_Components.FindStorage<TComponent>();
        return Storage == nullptr ? nullptr : Storage->TryGet(Value);
    }

    template<typename TComponent>
    TComponent& GetComponent(const Entity& Value) {
        TComponent* Found = TryGetComponent<TComponent>(Value);

        if (Found == nullptr) {
            throw std::out_of_range("GetComponent : composant absent de cette entite.");
        }

        return *Found;
    }

    template<typename TComponent>
    const TComponent& GetComponent(const Entity& Value) const {
        const TComponent* Found = TryGetComponent<TComponent>(Value);

        if (Found == nullptr) {
            throw std::out_of_range("GetComponent : composant absent de cette entite.");
        }

        return *Found;
    }

    // --- Vues ---

    template<typename... TComponents>
    View<TComponents...> GetView() {
        return View<TComponents...>(&m_Components.GetOrCreateStorage<TComponents>()...);
    }

    // --- Systèmes ---

    template<typename TSystem, typename... TArgs>
    TSystem& AddSystem(TArgs&&... Args) {
        return m_Systems.Add<TSystem>(*this, std::forward<TArgs>(Args)...);
    }

    void Update(float DeltaTime) { m_Systems.UpdateAll(*this, DeltaTime); }

    std::size_t GetSystemCount() const { return m_Systems.GetCount(); }

    // --- Nettoyage ---

    void Clear() {
        m_Systems.Clear(*this);
        m_Components.Clear();
        m_Entities.Clear();
    }

private:
    EntityManager m_Entities;
    ComponentManager m_Components;
    SystemManager m_Systems;
};
