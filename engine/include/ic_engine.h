/**
 * This is an external header that includes all the external headers for my engine
 */

#ifndef IC_ENGINE_H
#define IC_ENGINE_H

// core
#include "core/application.h"
#include "core/window.h"
#include "core/assets/asset_manager.h"
#include "core/input.h"
#include "core/filesystem.h"
#include "core/mousecodes.h"
#include "core/keycodes.h"

#include "core/ecs/entity.h"
#include "core/ecs/components.h"

#include "core/events/event.h"
#include "core/events/application_event.h"
#include "core/events/key_event.h"
#include "core/events/mouse_event.h"

#include "core/assets/types/model.h"

// renderer
#include "renderer/camera.h"
#include "renderer/renderer.h"
#include "renderer/scene.h"

#endif  // IC_ENGINE_H