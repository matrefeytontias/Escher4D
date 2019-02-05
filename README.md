# Escher4D
## Real-time slicing 4D engine experiment

This project includes code for creating, displaying and exploring a 4D space with 4D objects in it. So far it includes slicing-based display of 4D objects (no projection), correct 4D lighting, and I am working on real-time shadows.

The makefile makes this work on both Windows and Linux, **given your computer has an Nvidia GPU with OpenGL 4.4 on it**. I use extensions `GL_NV_gpu_shader5` and `GL_NV_shader_thread_group`.

### Instructions

Right now, this is basically a free-roaming demo. It lets you control a 4D camera in a room complex and explore as you wish. Use WASD (ZQSD on AZERTY keyboards) to move, the mouse to look around in 3D, and Q/E (A/E on AZERTY) to look around in 4D (try it out and see by yourself). There are no collisions yet, so you can go through the walls, but there isn't really much to see there.

### Building

- On Windows, you're going to need MSYS2 as found on [the official homepage](https://www.msys2.org/). Once that's installed, you'll need make and gcc : `pacman -S make gcc`. Once *that*'s installed, just run `make run` and that should be it.
- On Linux, you're going to need the GLFW3 runtime packages and that's about it. Once you do have that, `make run`.

### Credits

By Mattias Refeyton. This is part of my PRIM (Projet de Recherche et d'Innovation Master) ; end-of-3rd-year project at Télécom ParisTech.
