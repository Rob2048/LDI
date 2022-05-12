@REM PolyMender-clean.exe source.stl 9 0.9 voxel.ply 0.9
@REM "Instant Meshes.exe" voxel.ply -o output.ply --scale 0.06

PolyMender-clean.exe source.stl 10 0.9 voxel.ply
@REM Scale is edge length BEFORE quad subdivide. Therefore. (Units: cm) 0.01 = 50um output.
@REM "Instant Meshes.exe" voxel.ply -o output.ply --scale 0.02
@REM "Instant Meshes.exe" voxel.ply -o output.ply --scale 0.02

