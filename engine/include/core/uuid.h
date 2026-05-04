#ifndef UUID_H
#define UUID_H

#include "defines.h"

namespace ic
{

// TODO: Make a uuid generator
class UUID
{
public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID &) = default;

        operator uint64_t() const { return m_UUID; }

private:
        uint64_t m_UUID;
};

}  // namespace ic

namespace std
{
template <typename T>
struct hash;

template <>
struct hash<ic::UUID>
{
        std::size_t operator()(const ic::UUID &uuid) const { return (uint64_t)uuid; }
};

}  // namespace std

#endif  // UUID_H