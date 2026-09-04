#pragma once
#include "Entity.hpp"

#include <cstddef>
#include <vector>

class EntityManager {
public:
    Entity Add();
    void Remove(const Entity& Value);
    void Clear();

    bool IsAlive(const Entity& Value) const;
    std::size_t GetCount() const { return m_Entities.size(); }

    const std::vector<Entity>& GetEntities() const { return m_Entities; }

private:
    static constexpr std::size_t InvalidIndex = static_cast<std::size_t>(-1);

    std::vector<Entity> m_Entities;                     // liste dense des entités vivantes
    std::vector<Entity::GenerationType> m_Generations;  // indexée par id
    std::vector<std::size_t> m_DenseIndices;            // id -> index dans m_Entities
    std::vector<Entity::IdType> m_FreeIds;              // slots recyclables
};
