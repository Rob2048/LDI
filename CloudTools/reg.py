import time
import open3d as o3d
import numpy as np
import copy
import math

# TODO:
# https://geometry-central.net/surface/algorithms/geodesic_voronoi_tessellations/

#----------------------------------------------------------------------------------------------------------------------------------
# Helpers.
#----------------------------------------------------------------------------------------------------------------------------------
def preprocess_point_cloud(pcd, voxel_size):
	pcd_down = pcd.voxel_down_sample(voxel_size)

	radius_normal = voxel_size * 2
	# pcd_down.estimate_normals(o3d.geometry.KDTreeSearchParamHybrid(radius=radius_normal, max_nn=30))
	pcd_down.estimate_normals(o3d.geometry.KDTreeSearchParamHybrid(radius=1.0, max_nn=400))

	radius_feature = voxel_size * 5
	pcd_fpfh = o3d.pipelines.registration.compute_fpfh_feature(pcd_down, o3d.geometry.KDTreeSearchParamHybrid(radius=radius_feature, max_nn=100))
	return pcd_down, pcd_fpfh

def prepare_dataset(voxel_size, sourcePcd, targetPcd):
	source = sourcePcd
	target = targetPcd
	
	source_down, source_fpfh = preprocess_point_cloud(source, voxel_size)
	target_down, target_fpfh = preprocess_point_cloud(target, voxel_size)
	return source, target, source_down, target_down, source_fpfh, target_fpfh

def execute_global_registration(source_down, target_down, source_fpfh, target_fpfh, voxel_size):
	distance_threshold = voxel_size * 1.5
	result = o3d.pipelines.registration.registration_ransac_based_on_feature_matching(
		source_down, target_down, source_fpfh, target_fpfh, True,
		distance_threshold,
		o3d.pipelines.registration.TransformationEstimationPointToPoint(False),
		3, [
			o3d.pipelines.registration.CorrespondenceCheckerBasedOnEdgeLength(0.9),
			o3d.pipelines.registration.CorrespondenceCheckerBasedOnDistance(distance_threshold)
		], o3d.pipelines.registration.RANSACConvergenceCriteria(100000, 0.999))
	
	return result

#----------------------------------------------------------------------------------------------------------------------------------
# Main.
#----------------------------------------------------------------------------------------------------------------------------------
print("Starting mesh to scan registration.")

# Source mesh.
mesh = o3d.io.read_triangle_mesh("reg_input.ply")
mesh.compute_vertex_normals()

mesh_pcl = o3d.geometry.PointCloud()
mesh_pcl.points = o3d.utility.Vector3dVector(mesh.vertices)

# Scan point cloud.
scan_pcl = o3d.io.read_point_cloud("dwag_clipped.ply")

# Temporary downsample.
# scan_down_pcd = pcd.voxel_down_sample(voxel_size=0.01)
# scan_down_pcd.paint_uniform_color([0.6, 0.2, 0.65])
# scan_down_pcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=1.0, max_nn=400))

# o3d.visualization.draw_geometries([scan_down_pcd])

#----------------------------------------------------------------------------------------------------------------------------------
# Global registration.
#----------------------------------------------------------------------------------------------------------------------------------
voxel_size = 0.05
# TODO: Don't trample on normals from source mesh?
source, target, source_down, target_down, source_fpfh, target_fpfh = prepare_dataset(voxel_size, mesh_pcl, scan_pcl)

print("Apply global registration")
result_ransac = execute_global_registration(source_down, target_down, source_fpfh, target_fpfh, voxel_size)

source_down.paint_uniform_color([0.07, 0.73, 1.0])
target_down.paint_uniform_color([0.6, 0.2, 0.65])

# source_down.transform(result_ransac.transformation)
# source_down.translate((0, 0, 3))

# o3d.visualization.draw_geometries([source_down, target_down])

# exit()

# source_down.paint_uniform_color([0.07, 0.73, 1.0])
# target_down.paint_uniform_color([0.6, 0.2, 0.65])
# source_down.transform(result_ransac.transformation)
# o3d.visualization.draw_geometries([source_down, target_down])

#----------------------------------------------------------------------------------------------------------------------------------
# Local registration.
#----------------------------------------------------------------------------------------------------------------------------------
max_corr_distance = 0.1
trans_init = result_ransac.transformation
evaluation = o3d.pipelines.registration.evaluate_registration(source_down, target_down, max_corr_distance, trans_init)
print(evaluation)

print("Apply local registration")
reg_p2p = o3d.pipelines.registration.registration_icp(
	source_down, target_down, max_corr_distance, trans_init,
	o3d.pipelines.registration.TransformationEstimationPointToPoint(with_scaling=True),
	o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=10000))

trans_scale = np.linalg.norm(reg_p2p.transformation[[0, 1, 2], 0])
print("Scale: {}".format(trans_scale))

source_down.transform(reg_p2p.transformation)
mesh.transform(reg_p2p.transformation)

evaluation = o3d.pipelines.registration.evaluate_registration(source_down, target_down, max_corr_distance, np.identity(4))
print(evaluation)

# target_temp.translate((3, 0, 0))
# source_down.paint_uniform_color([0.07058821, 0.7273544, 1.0])
# target_down.paint_uniform_color([0.6, 0.2, 0.65])
# o3d.visualization.draw_geometries([source_down, target_down])

#----------------------------------------------------------------------------------------------------------------------------------
# Segmented registration
#----------------------------------------------------------------------------------------------------------------------------------
source_tree = o3d.geometry.KDTreeFlann(source_down)
marked_list = np.empty(len(source_down.points))
cluster_indices = []
cluster_transforms = []

# Generate cluster points.
for i in range(0, len(source_down.points)):
	if marked_list[i] == 1:
		continue
	
	search_result = source_tree.search_vector_3d(source_down.points[i], o3d.geometry.KDTreeSearchParamRadius(0.2))
	neighbours = search_result[1]

	found_seed = False
	for j in range(0, len(neighbours)):
		idx = neighbours[j]
		if marked_list[idx] == 1:
			continue
		
		found_seed = True
		marked_list[idx] = 1

	if found_seed:
		cluster_indices.append(i)

print("Cluster points: {}".format(len(cluster_indices)))

# Show local registration.
cluster_point_pcl = source_down.select_by_index(cluster_indices)
cluster_point_pcl.paint_uniform_color([0, 1, 0])
o3d.visualization.draw_geometries([cluster_point_pcl, mesh, target_down])

warped_clusters_pcl = []
max_corr_distance = 0.05

mesh_tree = o3d.geometry.KDTreeFlann(mesh)

# Register clusters.
for i in range(0, len(cluster_indices)):
	search_result = source_tree.search_vector_3d(source_down.points[cluster_indices[i]], o3d.geometry.KDTreeSearchParamRadius(0.2))
	cluster_pcl = source_down.select_by_index(search_result[1])
	
	# evaluation = o3d.pipelines.registration.evaluate_registration(cluster_pcl, target_down, max_corr_distance, np.identity(4))
	# print("Cluster pre fit: {}".format(evaluation))

	reg_p2p = o3d.pipelines.registration.registration_icp(cluster_pcl, target_down, max_corr_distance, np.identity(4),
		o3d.pipelines.registration.TransformationEstimationPointToPoint(with_scaling=False),
		o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=10000))
	
	# evaluation = o3d.pipelines.registration.evaluate_registration(cluster_pcl, target_down, max_corr_distance, np.identity(4))
	# print("Cluster post fit: {}".format(evaluation))

	cluster_pcl.transform(reg_p2p.transformation)
	cluster_transforms.append(reg_p2p.transformation)
	warped_clusters_pcl.append(cluster_pcl.points)

# Show warped clusters.
warped_clusters_pcl = np.concatenate(warped_clusters_pcl, axis=0)
warped_pcl = o3d.geometry.PointCloud()
warped_pcl.points = o3d.utility.Vector3dVector(warped_clusters_pcl)
warped_pcl.paint_uniform_color([0, 1, 0])
o3d.visualization.draw_geometries([warped_pcl, target_down, mesh])

#----------------------------------------------------------------------------------------------------------------------------------
# Deform source mesh.
#----------------------------------------------------------------------------------------------------------------------------------
# Build mesh rig.

# For every vert in source mesh, list of cluster influences.
influences = [None] * len(mesh.vertices)
for i in range(len(influences)):
	influences[i] = []

max_influence_dist = 0.3

# Gather all influences and weights per vertex.
for i in range(len(cluster_indices)):
	search_result = mesh_tree.search_radius_vector_3d(source_down.points[cluster_indices[i]], max_influence_dist)
	search_vert_ids = search_result[1]
	search_dists = search_result[2]

	for j in range(len(search_vert_ids)):
		weight = 1.0 - (math.sqrt(search_dists[j]) / max_influence_dist)
		influences[search_vert_ids[j]].append([i, weight])

# Normalize influence weights.
for i in range(len(influences)):
	influence_list = influences[i]

	if len(influence_list) == 0:
		continue
	
	influence_sum = 0
	for j in range(len(influence_list)):
		influence_sum = influence_sum + influence_list[j][1]
	
	for j in range(len(influence_list)):
		influence_list[j][1] = influence_list[j][1] / influence_sum

# Apply cluster transforms to verts.
for i in range(len(influences)):
	influence_list = influences[i]
	vert_homo = np.append(mesh.vertices[i], 1.0)
	final_vert = np.zeros(4, dtype=np.float64)

	if len(influence_list) == 0:
		final_vert = vert_homo
	else:
		for j in range(len(influence_list)):
			final_vert = final_vert + np.matmul(cluster_transforms[influence_list[j][0]], vert_homo) * influence_list[j][1]

	mesh.vertices[i] = final_vert[:3]

mesh.compute_vertex_normals()

o3d.io.write_triangle_mesh("output.ply", mesh)

o3d.visualization.draw_geometries([target_down, mesh, cluster_point_pcl])