#ifndef ENTITY_H
#define ENTITY_H

#include "defines.h"
#include <entt/entt.hpp>

/** I am not going to use some wrapper class for entity.
 * The sole reason for that is if entity provides these functions a little bit of repitition of code for writing the
 * interfaces does not even matter when the whole class is written by someone. Why should one use a wrapper for already
 * a high level designed object.
 */

/** Entity  */
using Entity = entt::entity;

#endif  // ENTITY_H