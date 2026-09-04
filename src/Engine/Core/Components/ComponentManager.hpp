#pragma once
#include "ComponentStorage.hpp"
#include <Engine/Core/Entities/Entity.hpp>
#include "IComponentStorage.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

class ComponentManager {
public:
    template<typename TComponent>
    ComponentStorage<TComponent>& GetOrCreateStorage() {
        const std::type_index Key(typeid(TComponent));
        const auto It = m_Storages.find(Key);

        if (It != m_Storages.end()) {
            return static_cast<ComponentStorage<TComponent>&>(*It->second);
        }

        auto Created = std::make_unique<ComponentStorage<TComponent>>();
        ComponentStorage<TComponent>& Reference = *Created;
        m_Storages.emplace(Key, std::move(Created));

        return Reference;
    }

    template<typename TComponent>
    ComponentStorage<TComponent>* FindStorage() {
        const auto It = m_Storages.find(std::type_index(typeid(TComponent)));

        if (It == m_Storages.end()) {
            return nullptr;
        }

        return static_cast<ComponentStorage<TComponent>*>(It->second.get());
    }

    template<typename TComponent>
    const ComponentStorage<TComponent>* FindStorage() const {
        const auto It = m_Storages.find(std::type_index(typeid(TComponent)));

        if (It == m_Storages.end()) {
            return nullptr;
        }

        return static_cast<const ComponentStorage<TComponent>*>(It->second.get());
    }

    void RemoveAll(const Entity& Value) {
        for (auto& Pair : m_Storages) {
            Pair.second->Remove(Value);
        }
    }

    void Clear() {
        for (auto& Pair : m_Storages) {
            Pair.second->Clear();
        }
    }

    std::size_t GetStorageCount() const { return m_Storages.size(); }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_Storages;
};
