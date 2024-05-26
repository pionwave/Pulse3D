# Pulse3D
Oldschool dos 3d rasterization engine. Full 6 degrees of freedom.

Currently the engine rasters in mode 13h - 320x200 and 256 colours.
Uses a back buffer, loading 256 colour BMP's as textures and rendering perspective correct triangles.

Z-sort is done via a BSP tree, there is a z-buffer but that isn't used yet - 
it will be used for sorting objects in the world against the pre-rendered world.

Next step is to create an external BSP compiler tool, using brush based maps.
Brush based maps are ideal as convex volumes make collision detection with a BSP
map super fast and simple; as anything in front of the split plane is in empty 
space, and anything behind the split plane is in solid space.
