#pragma once

#include <cstdint>

namespace ic
{

class UUID
{
public:
        UUID() = default;
        explicit UUID(uint64_t value) : m_Value(value) {}

        uint64_t Value() const { return m_Value; }

        operator uint64_t() const { return m_Value; }

        bool operator==(const UUID &other) const { return m_Value == other.m_Value; }

        bool operator!=(const UUID &other) const { return m_Value != other.m_Value; }

private:
        uint64_t m_Value = 0;
};

class UUIDGenerator
{
public:
        static UUID Generate();
};

}  // namespace ic

namespace std
{

template <>
struct hash<ic::UUID>
{
        size_t operator()(const ic::UUID &uuid) const { return static_cast<uint64_t>(uuid); }
};

}  // namespace std