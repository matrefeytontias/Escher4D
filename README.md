# Escher4D
## Real-time slicing 4D engine experiment

This project includes code for creating, displaying and exploring a 4D space with 4D objects in it. So far it includes slicing-based display of 4D objects (no projection), correct 4D lighting, and mostly-working real-time shadows.

The CMake config should make this work on any system **given your computer has an Nvidia GPU with OpenGL 4.4 on it**. I use extensions `GL_NV_gpu_shader5` and `GL_NV_shader_thread_group`. I have not tested this on anything other than MSVC++.

### Instructions

Right now, this is basically a free-roaming demo. It lets you control a 4D camera in a room complex and explore as you wish. Use WASD (ZQSD on AZERTY keyboards) to move, the mouse to look around in 3D, and Q/E (A/E on AZERTY) to look around in 4D (try it out and see by yourself). There are no collisions yet, so you can go through the walls, but there isn't really much to see there.

### Building

CMake baby :D

### Credits

By Mattias Refeyton. This started as my PRIM (Projet de Recherche et d'Innovation Master), the end-of-3rd-year project at Télécom ParisTech, but I'm still working on it.

This is using the libraries [Empty](https://github.com/matrefeytontias/Empty) (by me), [GLFW3](https://github.com/glfw/glfw), [imgui](https://github.com/ocornut/imgui) and [tetgen](https://wias-berlin.de/software/index.jsp?id=TetGen), all build from source.
