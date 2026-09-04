#pragma once

#include <Engine/Core/View.hpp>
#include <Engine/Core/Entities/Entity.hpp>

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

// Vue sur toutes les entités possédant simultanément TComponents...
// Le callback reçoit (Entity, TComponents&...).
template<typename... TComponents>
class View {
public:
    static_assert(sizeof...(TComponents) > 0, "Une vue doit porter sur au moins un composant.");

    explicit View(ComponentStorage<TComponents>*... Storages) : m_Storages(Storages...) {}

    bool IsValid() const {
        return std::apply(
            [](auto*... Storage) { return ((Storage != nullptr) && ...); },
            m_Storages);
    }

    // Taille du plus petit stockage : borne haute du nombre de résultats.
    std::size_t GetUpperBound() const {
        const IComponentStorage* Smallest = FindSmallest();
        return Smallest == nullptr ? 0 : Smallest->GetSize();
    }

    template<typename TFunction>
    void Each(TFunction&& Function) {
        if (!IsValid()) {
            return;
        }

        const IComponentStorage* Smallest = FindSmallest();

        // Copie volontaire : le callback peut détruire des entités ou retirer
        // des composants sans invalider l'itération. Les entités disparues en
        // cours de route sont écartées par HasAll.
        const std::vector<Entity> Owners = Smallest->GetOwners();

        for (const Entity& Current : Owners) {
            if (!HasAll(Current)) {
                continue;
            }

            Invoke(Function, Current);
        }
    }

    std::vector<Entity> Collect() const {
        std::vector<Entity> Result;

        if (!IsValid()) {
            return Result;
        }

        const IComponentStorage* Smallest = FindSmallest();

        for (const Entity& Current : Smallest->GetOwners()) {
            if (HasAll(Current)) {
                Result.push_back(Current);
            }
        }

        return Result;
    }

private:
    bool HasAll(const Entity& Value) const {
        return std::apply(
            [&Value](auto*... Storage) { return (Storage->Has(Value) && ...); },
            m_Storages);
    }

    template<typename TFunction>
    void Invoke(TFunction& Function, const Entity& Value) {
        std::apply(
            [&](auto*... Storage) { Function(Value, *Storage->TryGet(Value)...); },
            m_Storages);
    }

    const IComponentStorage* FindSmallest() const {
        const IComponentStorage* Result = nullptr;

        std::apply(
            [&Result](auto*... Storage) {
                ((Result = (Result == nullptr || Storage->GetSize() < Result->GetSize())
                    ? static_cast<const IComponentStorage*>(Storage)
                    : Result), ...);
            },
            m_Storages);

        return Result;
    }

    std::tuple<ComponentStorage<TComponents>*...> m_Storages;
};
