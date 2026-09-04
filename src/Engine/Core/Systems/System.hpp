#pragma once
class World;

class System {
public:
    virtual ~System() = default;

    System(const System&) = delete;
    System& operator=(const System&) = delete;

    virtual void OnAttach(World& Owner) { (void)Owner; }
    virtual void OnUpdate(World& Owner, float DeltaTime) = 0;
    virtual void OnDetach(World& Owner) { (void)Owner; }

    void SetEnabled(bool Value) { m_Enabled = Value; }
    bool IsEnabled() const { return m_Enabled; }

protected:
    System() = default;

private:
    bool m_Enabled = true;
};
