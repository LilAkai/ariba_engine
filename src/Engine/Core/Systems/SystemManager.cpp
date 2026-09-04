#include "SystemManager.hpp"

#include <Engine/Core/World.hpp>

void SystemManager::UpdateAll(World& Owner, float DeltaTime) {
    // Index et non itérateur : un système peut en ajouter un autre pendant
    // l'update sans faire exploser l'itération.
    for (std::size_t Index = 0; Index < m_Systems.size(); ++Index) {
        System& Current = *m_Systems[Index];

        if (Current.IsEnabled()) {
            Current.OnUpdate(Owner, DeltaTime);
        }
    }
}

void SystemManager::Clear(World& Owner) {
    for (std::size_t Index = m_Systems.size(); Index > 0; --Index) {
        m_Systems[Index - 1]->OnDetach(Owner);
    }

    m_Systems.clear();
}
