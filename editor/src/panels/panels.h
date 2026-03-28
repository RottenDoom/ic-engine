#ifndef PANELS_H
#define PANELS_H

#include <ic_engine.h>

namespace ic::panels
{
// Command pallete commands registration
// void register_commands();

void heirarchy_draw(ic::RenderScene *scene, ic::Entity &selected);

void component_panel_draw(ic::Entity &selected);

// menu bar should be inside the editor itself instead of a panel
// void menu_bar_draw();

// struct Command
// {
//         string name;
//         string shortcut;
//         void (*action)(void);
// };

// typedef void(action_command)(void);

// void command_palette_input();
// void command_palette_draw();

// void command_register(const char *name, const char *shortcut, action_command fn);

// // Profiler
// void profiler_draw();
// void profiler_begin(const char *name);
// void profiler_end(const char *name);

}  // namespace ic::panels

#endif  // PANELS_H