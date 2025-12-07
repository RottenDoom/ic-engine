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

- TODO: OpenGL pipeline and filesystem for better model and shader loading.
- TODO: Make builds for both shared and static libraries since I am going to test out both thus going to use both.
- TODO: Use ifdefs and defines better.
- Export API after creating model loading for users to use in the library
- DLL imports and exports and live reloads {CURR}
- Start rendering meshes {CURR}
- Vector Template for printing in defines.
- Create an actual game
- Workflows and Tests
- Make a release.
- Create a custom allocator that also makes a report on how much memory each object is using and if there is a memory leak.
- Write my own move functions and my own copy functions.
- Custom file system

#### Issues

- Modularity

The project is largely based on the game engine by the cherno.<p>
