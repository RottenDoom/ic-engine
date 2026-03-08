#ifndef ITERATOR_H
#define ITERATOR_H

#include "defines.h"

template <typename TypeA, typename TypeB>
using Pair = std::pair<TypeA, TypeB>;

/** @brief Generic iterator wrapper.
 * @example
 * using InternalIterator = std::unordered_map<GUID, IAsset*>::iterator;
 * using Iterator = MapIterator<InternalIterator, GUID, IAsset*>;
 */
template <typename InternalIterator, typename Key, typename Value>
class MapIterator
{
public:
        struct Entry
        {
                Key key;
                Value value;
        };

        MapIterator(InternalIterator it) : it_(it) {}

        MapIterator &operator++()
        {
                ++it_;
                return *this;
        }

        bool operator!=(const MapIterator &other) const { return it_ != other.it_; }

        Entry operator*() const { return {it_->first, it_->second}; }

private:
        InternalIterator it_;
};

#endif