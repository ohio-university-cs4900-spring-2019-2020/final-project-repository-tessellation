# Continuous Terrain Level of Detail by Tessellation Shaders
## About
This project demonstrates the capabilities (and pitfalls) involved with performing continuous level of detail on the GPU via tessellation shaders. Specifically, this is a demo of rendering the whole Earth with displacement mapping from an elevation dataset. The surface is also textured using a texture map of the Earth.

## Controls
- The camera controls are the default of the AftrBurnerEngine. Thus, left click and drag to rotate the camera, right click to move the camera forwards, and right click + hold shift to move the camera backwards. Use the scroll wheel to increase or decrease camera movement speed.
- Press the **space bar** to reorient the camera's movement axis so that it matches the current location on the Earth's surface. This makes the camera seem like it is positioned at that location on the Earth's surface (it's view reflects what a person would see standing there on the Earth).
- Press the **r key** to reset the camera's movement axis to the default.
- Press the **down arrow** to decrease the scale of the Earth, and the **up arrow** to increase it.
- Press the **left arrow** to decrease the tessellation factor, and the **right arrow** to increase it.
- Press the **i key** to decrease the maximum tessellation factor, and the **o key** to increase it.
- Press the **1 key** to toggle between rendering the Earth as a wireframe and as triangles.

## Cloning and Building
In order to build this project, the files in the engine subdirectory must be installed in the AftrBurnerEngine engine directory, and the engine must be rebuilt and installed. The files in the usr subdirectory must be installed in the AftrBurnerEngine usr directory, and then the EarthTessellationModule located in usr/modules/EarthTessellationModule can be configured and built.

To run the resulting module program, some data files need to be downloaded and installed. See [here](usr/modules/EarthTessellationModule/mm/images/README.md) for more details.