#pragma once
#include <Engine/Core/Entities/Entity.hpp>

#include <cstddef>
#include <vector>

// Interface non typée : c'est elle qui permet au ComponentManager de stocker
// des ComponentStorage<T> de types différents dans un même conteneur.
class IComponentStorage {
public:
    virtual ~IComponentStorage() = default;

    virtual bool Has(const Entity& Value) const = 0;
    virtual void Remove(const Entity& Value) = 0;
    virtual void Clear() = 0;
    virtual std::size_t GetSize() const = 0;

    // Liste dense des entités possédant ce composant.
    // Sert aux vues pour choisir le plus petit stockage comme pivot d'itération.
    virtual const std::vector<Entity>& GetOwners() const = 0;
};
