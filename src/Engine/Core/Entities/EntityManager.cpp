#include "EntityManager.hpp"

Entity EntityManager::Add() {
    if (m_Generations.empty()) {
        // Le slot 0 est réservé pour que l'id 0 signifie toujours "invalide".
        m_Generations.push_back(0);
        m_DenseIndices.push_back(InvalidIndex);
    }

    Entity::IdType Id;

    if (!m_FreeIds.empty()) {
        Id = m_FreeIds.back();
        m_FreeIds.pop_back();
    } else {
        Id = static_cast<Entity::IdType>(m_Generations.size());
        m_Generations.push_back(0);
        m_DenseIndices.push_back(InvalidIndex);
    }

    const Entity Created(Id, m_Generations[Id]);

    m_DenseIndices[Id] = m_Entities.size();
    m_Entities.push_back(Created);

    return Created;
}

void EntityManager::Remove(const Entity& Value) {
    if (!IsAlive(Value)) {
        return;
    }

    const Entity::IdType Id = Value.GetId();
    const std::size_t Index = m_DenseIndices[Id];
    const Entity Last = m_Entities.back();

    // Swap-remove : on bouche le trou avec la dernière entité.
    m_Entities[Index] = Last;
    m_DenseIndices[Last.GetId()] = Index;
    m_Entities.pop_back();

    m_DenseIndices[Id] = InvalidIndex;
    ++m_Generations[Id];
    m_FreeIds.push_back(Id);
}

void EntityManager::Clear() {
    for (const Entity& Current : m_Entities) {
        ++m_Generations[Current.GetId()];
        m_DenseIndices[Current.GetId()] = InvalidIndex;
        m_FreeIds.push_back(Current.GetId());
    }

    m_Entities.clear();
}

bool EntityManager::IsAlive(const Entity& Value) const {
    return Value.IsValid()
        && Value.GetId() < m_Generations.size()
        && m_Generations[Value.GetId()] == Value.GetGeneration();
}
