1. Make sure I can slowly start creating games using the API exposition
2. Use IMGUI and make a switch that turns the UI off entirely
3. Complete the material system since I cant load everything for now
4. Write better shaders and better material system for my engine

### CURRENT OBJECTIVES:
- [x] Scene saving and renaming entities. (Better serialization) (Make tests I guess)
- [x] Skybox
- [x] Light mesh (Now add material to it)
- [ ] Material Edit panel and skybox + shadow mapping.
- [ ] Camera panels. (#PROBABLY: Collections like blender for entities)
- [ ] Scene testing and sponza [Secondary: Add file compression in asset deserialization]
- [ ] Profiling and meshoptimizer.

### TODO:
- [x] Skybox and HDR and scene plane and RenderBox (or whatever you call it)
- [ ] Issue with multiple materials the unwrapped UVs get attached to other items.
- [ ] Search files using paths
- [ ] #include support (for reuse across many shaders), you need to implement it on the C++ side — read the file, string-replace #include "x" with the content of x, then pass the final concatenated string to glShaderSource. That's how engines like Godot and Unity handle it.
- [ ] Add camera properties in view i guess.
- [ ] Change IMGUI panel colors and change fonts
  - [ ] Basic materials panels
  - [x] Lighting system panel
- [ ] Texture plane system
- [ ] Make a good default asset directory structure.
- [ ] Scene camera


### ISSUES
- [ ] [CURR] Multiple ref counts for just loaded models
- [ ] [CURR] Loading multiple scenes causes to ref count to increase does not unloads the model.
- [ ] When save as just example.scene we get an error.
- [ ] Exception handling.
- [ ] Finding bugs everywhere needs testing.
- [ ] Fix the infinite mkdir exception or handle it better came from the part where I was not checking if a file was a directory and only checking it was a file path.

- [x] [CURR] Can't save scenes makes some wrangled string
- [x] Memory leak in fs_getParentPath when creating multiple scenes
- [x] Some pathing issues on saving the scene.
- [x] When creating new entities on clicking add mesh component we get exception.