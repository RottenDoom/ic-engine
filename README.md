# IC Engine
Sounds like a cool combustion engine. This engine is renders stuff so that I can creat games.

## Build commands

To build the project use:

```bash
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -B build
cmake --build build
```
You can use whatever build type you like, I prefer ninja.<p>
This builds the project and dll file with it. Havent used hot reloading yet.<p>

```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -B build-ninja
cmake --build build-ninja
```

### TODO
- Add debug and release builds
- DLL imports and exports and live reloads
- Start rendering meshes
- Create an actual game
- Workflows and Tests

The project is largely based on the game engine by the cherno and kofi engine.<p>
Thank You.