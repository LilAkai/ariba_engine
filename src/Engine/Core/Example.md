```Cpp
#include "World.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// --- Components ---
struct Position {
    float X = 0.0f;
    float Y = 0.0f;
};

struct Velocity {
    float X = 0.0f;
    float Y = 0.0f;
};

struct Health {
    int Current = 100;
    int Max = 100;
};

struct Name {
    std::string Value;
};

// --- Systems ---
class MovementSystem final : public System {
public:
    void OnUpdate(World& Owner, float DeltaTime) override {
        Owner.GetView<Position, Velocity>().Each(
            [DeltaTime](Entity Current, Position& Pos, Velocity& Vel) {
                (void)Current;
                Pos.X += Vel.X * DeltaTime;
                Pos.Y += Vel.Y * DeltaTime;
            });
    }
};

class DamageSystem final : public System {
public:
    explicit DamageSystem(int AmountPerSecond) : m_AmountPerSecond(AmountPerSecond) {}

    void OnUpdate(World& Owner, float DeltaTime) override {
        const int Amount = static_cast<int>(m_AmountPerSecond * DeltaTime);

        Owner.GetView<Health>().Each(
            [&Owner, Amount](Entity Current, Health& Hp) {
                Hp.Current -= Amount;

                if (Hp.Current <= 0) {
                    Owner.DestroyEntity(Current);
                }
            });
    }

private:
    int m_AmountPerSecond;
};

// --- Demonstration ---
int main() {
    World Game;

    Game.AddSystem<MovementSystem>();
    Game.AddSystem<DamageSystem>(40);

    const Entity Player = Game.CreateEntity();
    Game.AddComponent<Name>(Player, Name{"Joueur"});
    Game.AddComponent<Position>(Player, Position{0.0f, 0.0f});
    Game.AddComponent<Velocity>(Player, Velocity{2.0f, 1.0f});
    Game.AddComponent<Health>(Player, Health{200, 200});

    const Entity Wall = Game.CreateEntity();
    Game.AddComponent<Name>(Wall, Name{"Mur"});
    Game.AddComponent<Position>(Wall, Position{10.0f, 0.0f});

    const Entity Fragile = Game.CreateEntity();
    Game.AddComponent<Name>(Fragile, Name{"Fragile"});
    Game.AddComponent<Health>(Fragile, Health{30, 30});

    std::cout << "Entites au demarrage : " << Game.GetEntityCount() << "\n";

    for (int Frame = 0; Frame < 3; ++Frame) {
        Game.Update(1.0f);

        const Position& PlayerPos = Game.GetComponent<Position>(Player);
        std::cout << "Frame " << Frame
                  << " | joueur (" << PlayerPos.X << ", " << PlayerPos.Y << ")"
                  << " | pv " << Game.GetComponent<Health>(Player).Current
                  << " | entites " << Game.GetEntityCount() << "\n";
    }

    std::size_t Movable = 0;
    Game.GetView<Position, Velocity>().Each(
        [&Movable](Entity, Position&, Velocity&) { ++Movable; });
    assert(Movable == 1);

    // Fragile est mort en frame 0 : son handle est rejeté partout.
    assert(!Game.IsAlive(Fragile));
    assert(!Game.HasComponent<Health>(Fragile));
    assert(Game.TryGetComponent<Name>(Fragile) == nullptr);

    // Recyclage : la nouvelle entité réutilise l'id libéré, mais l'ancien
    // handle ne peut pas la toucher grâce à la génération.
    const Entity Recycled = Game.CreateEntity();
    assert(Recycled.GetId() == Fragile.GetId());
    assert(Recycled.GetGeneration() != Fragile.GetGeneration());
    Game.AddComponent<Name>(Recycled, Name{"Recyclee"});
    assert(Game.TryGetComponent<Name>(Fragile) == nullptr);
    assert(Game.GetComponent<Name>(Recycled).Value == "Recyclee");

    // Écrasement d'un composant existant.
    Game.AddComponent<Position>(Wall, Position{99.0f, 99.0f});
    assert(Game.GetComponent<Position>(Wall).X == 99.0f);

    // Retrait ciblé.
    Game.RemoveComponent<Position>(Wall);
    assert(!Game.HasComponent<Position>(Wall));

    // Une entité morte refuse tout ajout de composant.
    bool Threw = false;

    try {
        Game.AddComponent<Position>(Fragile, Position{});
    } catch (const std::invalid_argument&) {
        Threw = true;
    }

    assert(Threw);

    // Charge : création puis destruction en masse.
    std::vector<Entity> Batch;
    Batch.reserve(1000);

    for (int Index = 0; Index < 1000; ++Index) {
        const Entity Current = Game.CreateEntity();
        Game.AddComponent<Position>(Current, Position{static_cast<float>(Index), 0.0f});

        if (Index % 2 == 0) {
            Game.AddComponent<Velocity>(Current, Velocity{1.0f, 0.0f});
        }

        Batch.push_back(Current);
    }

    std::size_t Paired = 0;
    Game.GetView<Position, Velocity>().Each(
        [&Paired](Entity, Position&, Velocity&) { ++Paired; });
    assert(Paired == 501);  // 500 du lot + le joueur

    for (const Entity& Current : Batch) {
        Game.DestroyEntity(Current);
    }

    for (const Entity& Current : Batch) {
        assert(!Game.IsAlive(Current));
        assert(!Game.HasComponent<Position>(Current));
    }

    std::cout << "Entites restantes : " << Game.GetEntityCount() << "\n";

    Game.Clear();
    assert(Game.GetEntityCount() == 0);
    assert(Game.GetSystemCount() == 0);

    std::cout << "Tous les tests passent.\n";

    return 0;
}
```