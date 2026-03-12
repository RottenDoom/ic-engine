#ifndef UUID_H
#define UUID_H

#include "defines.h"

namespace ic
{
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

#endif  // UUID_H