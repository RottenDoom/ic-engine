# ic-engine
My Game Engine Project. This game engine uses vulkan api for rendering and I plan to make my own game in this engine.

# TODOs
1. The input system is rigged. The problem stems from usage of pointer functions. Since I used code from Kohi engine I can't figure out how I should integrate C++ object oriented programming with C programming style. What happens then is that if I don't use class varible and use statically defined variable for my app. I can't do anything since I get a segmentation fault. And if I use this my function pointers complain all the time. Also I can't use the functions directly into the register events since the function I made doesn't matches my pfn. Which just sucks. 

2. After completing input start with renderer.

3. Continue the Kohi Game engine series. I do not want to make my own math library so I am going to use glm. Probably gonna make another branch in my tutorial github repo to test it.


Solution to todo 1 is I am going to use layers and layerstack and use my own event class with dispatcher to create my own event system which just works that well. I just hope it works I know it will take a lot of time to create.