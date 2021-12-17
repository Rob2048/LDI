PolyMender program for robustly repairing a polygonal model.

Author: Tao Ju (taoju@cs.wustl.edu)

Version: 1.7.1 (Updated: May 20, 2011)

Platform: Windows


I. History

New in v1.7:

> Controlled removal of disconnected components

> Bug fixed in generating manifold surfaces

> Add option to specify bounding box


New in v1.6:

> Removal of disconnected components

> Topologically manifold dual contouring

> Output signed octree with geometry



II. Introduction


PolyMender is based on the algorithm presented in the paper "Robust Repair of Polygonal Models" (SIGGRAPH 2004). The program reads in a polygonal model (i.e., a bag of polygons) and produces a closed surface that approximates the original model. PolyMender consumes a small amount of time and memory space, and can accurately reproduce the original geometry. PolyMender is suitable for repairing CAD models and gigantic polygonal models. Alternatively, PolyMender can also be used to generate a signed volume from any polygonal models.



III. How to run


The executable package contains four programs:

1. PolyMender, PolyMender-clean

Purpose: repairs general purpose models, such as those created from range scanners. The repaired surface is constructed using Marching Cubes. Consumes minimal memory and time and generates closed, manifold triangular surfaces. The -clean option removes isolated pieces. You can control the size of components to be removed as a ratio of the largest component.

2. PolyMender-dc, PolyMender-dc-clean

Purpose: repairs models containing sharp features, such as CAD models. The repaired surface is constructed using Dual Contouring with a manifold topology, which is capable of reproducing sharp edges and corners. However, more memory is required. Generates closed triangular and quadrilateral surfaces. The -clean option removes isolated pieces.

3. PolyMender-qd, PolyMender-qd-clean

Purpose: same as the -dc and -dc-clean options, except that the sharp features are no longer reproduced in the output. Compared to -dc option, the -qd outputs tend to have fewer intersecting triangles and more regularly shaped quads. Under the hood, the vertex within each grid cell is placed at QEF minimizer in the -dc option, but at the centroid of the edge intersections in the -qd option.


Type the program names (e.g., PolyMender) on the DOS prompt and you will see their usages:

Usage:   PolyMender <input_file> <octree_depth> <scale> <output_file> [<thresh>] [<lowx> <lowy> <lowz> <size>]

Example: PolyMender bunny.ply 6 0.9 closedbunny.ply

Description:

<input_file>    Polygonal file of format STL (binary only), ASC, or PLY.

<octree_depth>  Integer depth of octree. The dimension of the volumetric
                grid is 2^<octree_depth> on each side.

<scale>         Floating point number between 0 and 1 denoting the ratio of
                the largest dimension of the model over the size of the grid.

<output_file>   Output in polygonal format PLY, signed-octree format SOF (or SOG), or dual contouring format DCF. SOF/SOG/DCF is only available for -dc and -qd options.

<thresh>	Minimum size of components to preserve, as a ratio of the number of polygons in the largest component. (default = 1.0)

<lowx(y,z)>	Coordinates of the lower corner of the user-specified bounding box

<size>		Length of the user-specified bounding box



Additional notes:

1. STL(binary) is preferred input format, since the program does not need to store the model in memory at all. ASC or PLY formats require additional storage of vertices, due to their topology-geometry file structure.

2. The running time and memory consumption of the program depends on several factors: the number of input polygons, the depth of the octree, and the surface area of the model (hence the number of leaf nodes on the octree). To give an idea, processing the David model with 56 million triangles at depth 13 takes 45 minutes using 500 MB RAM (excluding the mem allocated for storing vertices when reading PLY format) on a PC with AMD 1.5Hz CPU.

3. The number of output polygons can be finely controlled using the scale argument. The large the scale, the more polygons are generated, since the model occupies a larger portion of the volume grid.

4. As an alternative of output repaired models, the intermediate signed octree can be generated as a SOF or SOG file. The signed octree can be used for generating signed distance field, extracting isosurfaces, or multiresolution spatial representation of the polygonal model.

5. The user-specified bounding box is considered only if 1) the box size is no smaller than the model's own bounding box, and 2) the lowx(y,z) is no larger than the corner coordinates of the model's own bounding box.

6. The program currently works on only 32-bit machines. If you need to run on 64-bit machines, please send me an email and I can give you a 64-bit version.


IV SOF format

SOF (Signed Octree Format) records an octree grid with signes attached to the 8 corners of each leaf node. All leaf nodes appear at the same depth that is specified by the <octree_depth> argument to the program. The tree is recorded in SOF file using pre-order traversal. Here is the structure of a SOF file (binary):

<header>

<node>

<header> is a 4-bytes integer that equals 2 ^ octree_depth. The first byte of a <node> is either 0 (denoting an intermediate node) or 1 (denoting an empty node) or 2 (denoting a leaf node). After the first byte, an intermediate node <node> contains eight <node> structures for its eight children; an empty node <node> contains one byte of value 0 or 1 denoting if it is inside or outside; and a leaf node contains one byte whose eight bits correspond to the signs at its eight corners (0 for inside and 1 for outside). The order of enumeration of the eight children nodes in an intermediate nodeis the following (expressed in coordinates <x,y,z> ): <0,0,0>,<0,0,1>,<0,1,0>,<0,1,1>,<1,0,0>,<1,0,1>,<1,1,0>,<1,1,1>. The enumeration of the eight corners in a leaf node follows the same order (e.g., the lowest bit records the sign at <0,0,0>).



V SOG format

SOG (Signed Octree with Geometry) has the same data structure with SOF, with the addition of following features:

1. The file starts with a 128-byte long header. Currently, the header begins with the string "SOG.Format 1.0" followed by 3 floats representing the lower-left-near corner of the octree follwed by 1 float denoting the length of the octree (in one direction). The locations and lengths are in the input model's coordinate space. The rest of the header is left empty.

2. Each leaf node has additioanl three floats {x,y,z} (following the signs) denoting the geometric location of a feature vertex within the cell.



VI DCF format

DCF (Dual Contouring Format) records an octree grid where each non-empty lead node is associated with signs at the 8 corners and intersection points with normals at the 12 edges. All leaf nodes appear at the same depth that is specified by the <octree_depth> argument to the program. The tree is recorded in pre-order traversal. Here is the structure of a DCF file (binary):

<header>

<node>

<header> starts with a 10-byte string with content "multisign", followed by 3 4-byte integers describing the length of the grid in each direction (for an octree, they would all be 2 ^ octree_depth). The first 4-byte of a <node> is an integer of value either 0 (denoting an intermediate node) or 1 (denoting an empty node) or 2 (denoting a leaf node). After this, an intermediate node <node> contains eight <node> structures for its eight children; an empty node <node> contains a 2-byte short integer of value 0 or 1 denoting if it is inside or outside; and a leaf node contains 8 2-byte short integers indicating the signs at the eight corners (0 for inside and 1 for outside) and, for each of the 12 edges, a 4-byte integer indicating the number of points on that edge and, for each point, a float value that is the offset of the point from the lower-left-near end of the edge and three floats that describe the normal at the point. The order of enumeration of the eight children nodes in an intermediate node (and eight corners in a leaf node), expressed in coordinates <x,y,z>, is: <0,0,0>, <0,0,1>, <0,1,0>, <0,1,1>, <1,0,0>, <1,0,1>, <1,1,0>, <1,1,1>. The ordering of 12 edges in a leaf node, expressed using indices of the corners, is: {0,4}, {1,5}, {2,6}, {3,7}, {0,2}, {1,3}, {4,6}, {5,7}, {0,1}, {2,3}, {4,5}, {6,7}.



VII Test data

Three models are included in the testmodels package. (Suggested arguments are provided in the parathesis).

bunny.ply (octree depth: 7, scale: 0.9)

- The Stanford Bunny (containing big holes at the bottom)

horse.stl (octree depth: 8, scale: 0.9)

- The horse model with 1/3 of all polygons removed and vertices randomly perturbed.

mechanic.asc (octree depth: 6, scale: 0.9)

- A mechanic part with hanging triangles
