#pragma once
#include <cstdint>
#include <functional>

class Entity {
public:
    using IdType = std::uint32_t;
    using GenerationType = std::uint32_t;

    static constexpr IdType InvalidId = 0;

    Entity() = default;
    Entity(IdType Value, GenerationType Gen) : Id(Value), Generation(Gen) {}

    IdType GetId() const { return Id; }
    GenerationType GetGeneration() const { return Generation; }
    bool IsValid() const { return Id != InvalidId; }

    bool operator==(const Entity& Other) const {
        return Id == Other.Id && Generation == Other.Generation;
    }

    bool operator!=(const Entity& Other) const {
        return !(*this == Other);
    }

private:
    IdType Id = InvalidId;
    GenerationType Generation = 0;
};

namespace std {
    template<>
    struct hash<Entity> {
        std::size_t operator()(const Entity& Value) const noexcept {
            const std::uint64_t Packed =
                (static_cast<std::uint64_t>(Value.GetGeneration()) << 32) |
                static_cast<std::uint64_t>(Value.GetId());
            return std::hash<std::uint64_t>{}(Packed);
        }
    };
}
