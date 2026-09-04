#pragma once
#include "System.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class World;

class SystemManager {
public:
    // Les systèmes s'exécutent dans leur ordre d'ajout.
    template<typename TSystem, typename... TArgs>
    TSystem& Add(World& Owner, TArgs&&... Args) {
        static_assert(std::is_base_of<System, TSystem>::value,
                      "TSystem doit dériver de System.");

        auto Created = std::make_unique<TSystem>(std::forward<TArgs>(Args)...);
        TSystem& Reference = *Created;
        m_Systems.push_back(std::move(Created));

        Reference.OnAttach(Owner);

        return Reference;
    }

    void UpdateAll(World& Owner, float DeltaTime);
    void Clear(World& Owner);

    std::size_t GetCount() const { return m_Systems.size(); }

private:
    std::vector<std::unique_ptr<System>> m_Systems;
};
