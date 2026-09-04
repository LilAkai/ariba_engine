#pragma once
#include "IComponentStorage.hpp"

#include <cstddef>
#include <utility>
#include <vector>

// Sparse set : les composants vivent dans un vector dense et contigu
// (parcours rapide), et un tableau creux indexé par id d'entité donne
// l'accès direct en O(1).
template<typename TComponent>
class ComponentStorage final : public IComponentStorage {
public:
    static constexpr std::size_t InvalidIndex = static_cast<std::size_t>(-1);

    // Ajoute le composant, ou écrase celui déjà présent.
    template<typename... TArgs>
    TComponent& Emplace(const Entity& Owner, TArgs&&... Args) {
        EnsureSparseSize(Owner.GetId());

        const std::size_t Existing = m_Sparse[Owner.GetId()];

        if (Existing != InvalidIndex) {
            if (m_Owners[Existing] == Owner) {
                m_Components[Existing] = TComponent(std::forward<TArgs>(Args)...);
                return m_Components[Existing];
            }

            // Entrée orpheline laissée par une entité morte dont l'id a été
            // recyclé. On la purge avant de réutiliser le slot.
            RemoveAt(Existing);
        }

        m_Sparse[Owner.GetId()] = m_Components.size();
        m_Owners.push_back(Owner);
        m_Components.emplace_back(std::forward<TArgs>(Args)...);

        return m_Components.back();
    }

    TComponent* TryGet(const Entity& Value) {
        const std::size_t Index = FindIndex(Value);
        return Index == InvalidIndex ? nullptr : &m_Components[Index];
    }

    const TComponent* TryGet(const Entity& Value) const {
        const std::size_t Index = FindIndex(Value);
        return Index == InvalidIndex ? nullptr : &m_Components[Index];
    }

    bool Has(const Entity& Value) const override {
        return FindIndex(Value) != InvalidIndex;
    }

    void Remove(const Entity& Value) override {
        const std::size_t Index = FindIndex(Value);

        if (Index != InvalidIndex) {
            RemoveAt(Index);
        }
    }

    void Clear() override {
        m_Components.clear();
        m_Owners.clear();
        m_Sparse.clear();
    }

    std::size_t GetSize() const override { return m_Components.size(); }

    const std::vector<Entity>& GetOwners() const override { return m_Owners; }

    std::vector<TComponent>& GetComponents() { return m_Components; }
    const std::vector<TComponent>& GetComponents() const { return m_Components; }

private:
    void EnsureSparseSize(Entity::IdType Id) {
        if (Id >= m_Sparse.size()) {
            m_Sparse.resize(static_cast<std::size_t>(Id) + 1, InvalidIndex);
        }
    }

    // Vérifie l'id ET la génération : un handle périmé est rejeté.
    std::size_t FindIndex(const Entity& Value) const {
        if (!Value.IsValid() || Value.GetId() >= m_Sparse.size()) {
            return InvalidIndex;
        }

        const std::size_t Index = m_Sparse[Value.GetId()];

        if (Index == InvalidIndex || m_Owners[Index] != Value) {
            return InvalidIndex;
        }

        return Index;
    }

    void RemoveAt(std::size_t Index) {
        const Entity::IdType RemovedId = m_Owners[Index].GetId();
        const std::size_t Last = m_Components.size() - 1;

        if (Index != Last) {
            m_Components[Index] = std::move(m_Components[Last]);
            m_Owners[Index] = m_Owners[Last];
            m_Sparse[m_Owners[Index].GetId()] = Index;
        }

        m_Components.pop_back();
        m_Owners.pop_back();
        m_Sparse[RemovedId] = InvalidIndex;
    }

    std::vector<TComponent> m_Components;  // dense
    std::vector<Entity> m_Owners;          // dense, parallèle à m_Components
    std::vector<std::size_t> m_Sparse;     // id d'entité -> index dense
};
