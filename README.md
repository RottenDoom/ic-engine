# IC Engine

Sounds like a cool combustion engine. This engine renders stuff so that I can create games.

## Build commands

To build the project use:

```bash
build-all.bat gcc debug
```

OR

```bash
cmake --preset="gcc-debug"
cmake --build --preset="gcc-debug"
```

I am going to update the shaders later.

The executable can be found inside `build/build-gcc/bin/Debug/`. You can also use clang compiler now. I use vscode for now but configurations for other IDEs can be easily developed.

### TODO
- TODO: Make builds for both shared and static libraries since I am going to test out both thus going to use both.
- TODO: Use ifdefs and defines better.
- TODO: MultiThreading support and Threadpools module {CURR}
- TODO: Fix camera and new ECS system {CURR}
- Export API after creating model loading for users to use in the library
- Live reloads
- Vector Template for printing in defines.
- Create an actual game
- Workflows and Tests
- Make a release.
- Write my own move functions and my own copy functions.

#### Issues

- On making a directory that already exists the functions recursively keeps calling itself. Fix this issue.
- I mostly used normal malloc calls everywhere. Use maybe a better allocator so we can use it in lesser memory.
- Modularity

The project is largely based on the game engine by the cherno.<p>
