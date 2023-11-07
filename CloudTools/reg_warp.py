import time
import open3d as o3d
import numpy as np
import copy
import math

#----------------------------------------------------------------------------------------------------------------------------------
# Helpers.
#----------------------------------------------------------------------------------------------------------------------------------
def display_inlier_outlier(cloud, ind):
	inlier_cloud = cloud.select_by_index(ind)
	outlier_cloud = cloud.select_by_index(ind, invert=True)

	print("Showing outliers (red) and inliers (gray): ")
	outlier_cloud.paint_uniform_color([1, 0, 0])
	inlier_cloud.paint_uniform_color([0.8, 0.8, 0.8])
	o3d.visualization.draw_geometries([inlier_cloud, outlier_cloud])

def draw_registration_result_temp(source, target, transformation):
	source_temp = copy.deepcopy(source)
	target_temp = copy.deepcopy(target)
	source_temp.paint_uniform_color([0.07058821, 0.7273544, 1.0])
	target_temp.paint_uniform_color([0.6, 0.2, 0.65])
	source_temp.transform(transformation)
	# target_temp.translate((3, 0, 0))
	o3d.visualization.draw_geometries([source_temp, target_temp])
	# mesh.transform(transformation)
	# o3d.visualization.draw_geometries([source_temp, target_temp, mesh])

def preprocess_point_cloud(pcd, voxel_size):
	print(":: Downsample with a voxel size %.3f." % voxel_size)
	pcd_down = pcd.voxel_down_sample(voxel_size)

	radius_normal = voxel_size * 2
	print(":: Estimate normal with search radius %.3f." % radius_normal)
	pcd_down.estimate_normals(o3d.geometry.KDTreeSearchParamHybrid(radius=radius_normal, max_nn=30))

	radius_feature = voxel_size * 5
	print(":: Compute FPFH feature with search radius %.3f." % radius_feature)
	pcd_fpfh = o3d.pipelines.registration.compute_fpfh_feature(pcd_down, o3d.geometry.KDTreeSearchParamHybrid(radius=radius_feature, max_nn=100))
	return pcd_down, pcd_fpfh

def prepare_dataset(voxel_size, sourcePcd, targetPcd):
	print(":: Load two point clouds and disturb initial pose.")
	source = sourcePcd
	target = targetPcd
	# trans_init = np.asarray([[0.0, 0.0, 1.0, 0.0], [1.0, 0.0, 0.0, 0.0],
	# 						 [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]])
	# source.transform(trans_init)
	#draw_registration_result(source, target, np.identity(4))

	source_down, source_fpfh = preprocess_point_cloud(source, voxel_size)
	target_down, target_fpfh = preprocess_point_cloud(target, voxel_size)
	return source, target, source_down, target_down, source_fpfh, target_fpfh

def execute_global_registration(source_down, target_down, source_fpfh,
								target_fpfh, voxel_size):
	distance_threshold = voxel_size * 1.5
	print(":: RANSAC registration on downsampled point clouds.")
	print("   Since the downsampling voxel size is %.3f," % voxel_size)
	print("   we use a liberal distance threshold %.3f." % distance_threshold)
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
print("Point cloud test")

# Source mesh.
# mesh = o3d.io.read_triangle_mesh("modelFull.stl")
mesh = o3d.io.read_triangle_mesh("quad_mesh.ply")
mesh.compute_vertex_normals()
# mesh.paint_uniform_color([0.07058821, 0.7273544, 1.0])

# R = mesh.get_rotation_matrix_from_xyz((-np.pi / 2, 0, np.pi * 0))
# mesh.rotate(R, center=(0, 0, 0))
# mesh.scale(0.0468, center=(0, 0, 0))

# mesh_pcl = mesh.sample_points_poisson_disk(number_of_points=20000, init_factor=5)

# o3d.visualization.draw_geometries([mesh])

# mesh_segment = o3d.io.read_triangle_mesh("modelTail.stl")
# mesh_segment.compute_vertex_normals()
# mesh_segment.paint_uniform_color([0.8, 0.05, 0.1])

# R = mesh_segment.get_rotation_matrix_from_xyz((-np.pi / 2, 0, np.pi * 0))
# mesh_segment.rotate(R, center=(0, 0, 0))
# mesh_segment.scale(0.049, center=(0, 0, 0))

# # o3d.visualization.draw_geometries([mesh, mesh_segment])


# mesh_segment_cloud = mesh_segment.sample_points_poisson_disk(number_of_points=3000, init_factor=5)
# mesh_segment_cloud.paint_uniform_color([0.6, 0.2, 0.1])

# o3d.visualization.draw_geometries([mesh_pcl, mesh_segment_cloud])
# o3d.visualization.draw_geometries([mesh])

# mesh_pcl = o3d.io.read_point_cloud("quad_derg.ply")
mesh_pcl = o3d.geometry.PointCloud()
mesh_pcl.points = o3d.utility.Vector3dVector(mesh.vertices)

# o3d.visualization.draw_geometries([mesh, mesh_pcl])

# exit()

# Target mesh (scan).
pcd = o3d.io.read_point_cloud("dwag_clipped.ply")
# pcd = o3d.io.read_point_cloud("scanPoints.ply")
# print(pcd)

# o3d.visualization.draw_geometries([mesh_pcl, pcd])
# exit()

# cl, ind = pcd.remove_statistical_outlier(nb_neighbors=20, std_ratio=2.0)
# inlier_cloud = pcd.select_by_index(ind)
# display_inlier_outlier(pcd, ind)

# scan_down_pcd = inlier_cloud.voxel_down_sample(voxel_size=0.01)
scan_down_pcd = pcd.voxel_down_sample(voxel_size=0.01)
scan_down_pcd.paint_uniform_color([0.6, 0.2, 0.65])
# scan_down_pcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=0.1, max_nn=30))
# scan_down_pcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=0.06, max_nn=400))
scan_down_pcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=1.0, max_nn=400))

# print(scan_down_pcd)
# o3d.visualization.draw_geometries([scan_down_pcd])

# pcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=0.05, max_nn=200))
# pcd.paint_uniform_color([0.6, 0.2, 0.65])
# o3d.visualization.draw_geometries([pcd])

#----------------------------------------------------------------------------------------------------------------------------------
# Global registration.
#----------------------------------------------------------------------------------------------------------------------------------

source, target, source_down, target_down, source_fpfh, target_fpfh = prepare_dataset(0.05, mesh_pcl, scan_down_pcd)

# Draw features
# print(target_down)
# print(target_fpfh)

# o3d.pipelines.registration.CorrespondencesFromFeatures(source_fpfh, target_fpfh)

source_down.paint_uniform_color([0.07, 0.73, 1.0])
target_down.paint_uniform_color([0.6, 0.2, 0.65])

kdtree = o3d.geometry.KDTreeFlann(target_fpfh)

source_tree = o3d.geometry.KDTreeFlann(source_down)
res = source_tree.search_vector_3d(source_down.points[2005], o3d.geometry.KDTreeSearchParamRadius(0.2))

print(source_fpfh.data.shape)

for i in range(0, len(res[1])):
	source_point_id = res[1][i]
	source_down.colors[source_point_id] = [1, 0, 0]

	id, iv, dv = kdtree.search_vector_xd(source_fpfh.data[:33, source_point_id], o3d.geometry.KDTreeSearchParamKNN(100))
	for i in range(0, len(iv)):
		target_down.colors[iv[i]] = [1, 0, 0]

result_ransac = execute_global_registration(source_down, target_down,
											source_fpfh, target_fpfh,
											0.05)


# source_down.transform(result_ransac.transformation)
# source_down.translate((0, 0, 3))

# o3d.visualization.draw_geometries([source_down, target_down])

# exit()

# o3d.visualization.draw_geometries([scan_down_pcd, mesh_pcl])

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

print("Apply point-to-point ICP")
reg_p2p = o3d.pipelines.registration.registration_icp(
	source_down, target_down, max_corr_distance, trans_init,
	o3d.pipelines.registration.TransformationEstimationPointToPoint(with_scaling=True),
	o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=10000))

print(reg_p2p)
print("Transformation is:")

print(reg_p2p.transformation)
trans_scale = np.linalg.norm(reg_p2p.transformation[[0, 1, 2], 0])
print("Scale: {}".format(trans_scale))

# Move source.
# source_down.translate((0, 0, 6))

# draw_registration_result_temp(source_down, target_down, reg_p2p.transformation)
source_transd = copy.deepcopy(source_down)
source_transd.paint_uniform_color([0.07058821, 0.7273544, 1.0])
target_down.paint_uniform_color([0.6, 0.2, 0.65])
source_transd.transform(reg_p2p.transformation)
mesh.transform(reg_p2p.transformation)

t0 = time.time()
evaluation = o3d.pipelines.registration.evaluate_registration(source_transd, target_down, max_corr_distance, np.identity(4))
t1 = time.time()

print("Evaluation time: {}".format(t1 - t0))
print(evaluation)


# target_temp.translate((3, 0, 0))
# o3d.visualization.draw_geometries([source_transd, target_down])

#----------------------------------------------------------------------------------------------------------------------------------
# Segmented registration
#----------------------------------------------------------------------------------------------------------------------------------
# https://geometry-central.net/surface/algorithms/geodesic_voronoi_tessellations/

# o3d.visualization.draw_geometries([source_transd])

source_tree = o3d.geometry.KDTreeFlann(source_transd)
marked_list = np.empty(len(source_transd.points))

seed_points = []

for i in range(0, len(source_transd.points)):
	if marked_list[i] == 1:
		continue
	
	search_result = source_tree.search_vector_3d(source_transd.points[i], o3d.geometry.KDTreeSearchParamRadius(0.2))
	neighbours = search_result[1]

	color = list(np.random.random(size=3))

	found_seed = False

	for j in range(0, len(neighbours)):
		idx = neighbours[j]
		if marked_list[idx] == 1:
			continue
		
		found_seed = True
		marked_list[idx] = 1
		source_transd.colors[idx] = color

	if found_seed:
		seed_points.append(i)

seed_pcl = source_transd.select_by_index(seed_points)

o3d.visualization.draw_geometries([seed_pcl, mesh])

warped_pcl_points = []
max_corr_distance = 0.05

print("Seed points: {}".format(len(seed_points)))

cluster_transforms = []
cluster_indices = []

mesh_tree = o3d.geometry.KDTreeFlann(mesh)

# for i in range(1, 2):
for i in range(0, len(seed_points)):
	search_result = source_tree.search_vector_3d(source_transd.points[seed_points[i]], o3d.geometry.KDTreeSearchParamRadius(0.2))
	
	seg_pcl = source_transd.select_by_index(search_result[1])
	seg_pcl.paint_uniform_color([1, 0, 0])

	# evaluation = o3d.pipelines.registration.evaluate_registration(seg_pcl, target_down, max_corr_distance, np.identity(4))
	# print(evaluation)

	reg_p2p = o3d.pipelines.registration.registration_icp(seg_pcl, target_down, max_corr_distance, np.identity(4),
		o3d.pipelines.registration.TransformationEstimationPointToPoint(with_scaling=False),
		o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=10000))

	seg_pcl.transform(reg_p2p.transformation)

	# print(reg_p2p.transformation)

	cluster_transforms.append(reg_p2p.transformation)

	warped_pcl_points.append(seg_pcl.points)

	# Warp mesh vertex.
	# search_result = mesh_tree.search_knn_vector_3d(source_transd.points[seed_points[i]], 1)
	cluster_indices.append(seed_points[i])

	# vert_pos = mesh.vertices[search_result[1][0]]
	# vert4 = np.append(vert_pos, 1)
	# mesh.vertices[cluster_indices[i]] =  np.matmul(reg_p2p.transformation, vert4)[:3]
	# vert4 =  np.matmul(reg_p2p.transformation, vert4)[:3]

	# evaluation = o3d.pipelines.registration.evaluate_registration(seg_0_transd, target_down, max_corr_distance, np.identity(4))
	# print(evaluation)

	# o3d.visualization.draw_geometries([source_transd, target_down, seg_0_transd])

	# o3d.visualization.draw_geometries([seg_pcl, target_down])

warped_pcl_points = np.concatenate(warped_pcl_points, axis=0)
warped_pcl = o3d.geometry.PointCloud()
warped_pcl.points = o3d.utility.Vector3dVector(warped_pcl_points)
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
# for i in range(1):
	search_result = mesh_tree.search_radius_vector_3d(source_transd.points[cluster_indices[i]], max_influence_dist)

	search_vert_ids = search_result[1]
	search_dists = search_result[2]

	# print(search_result)

	for j in range(len(search_vert_ids)):
		# influences[search_vert_ids[j]].append([i, 0.3 - search_dists[j]])
		# influences[search_vert_ids[j]].append([i, 1.0 - search_dists[j] / max_influence_dist])

		inf = 1.0 - (math.sqrt(search_dists[j]) / max_influence_dist)
		influences[search_vert_ids[j]].append([i, inf])
		# print(inf, search_dists[j])
		

print(influences[33772])

# Normalize influence weights.
for i in range(len(influences)):
	influence_list = influences[i]
	influence_sum = 0

	for j in range(len(influence_list)):
		influence_sum = influence_sum + influence_list[j][1]

	for j in range(len(influence_list)):
		if influence_sum == 0:
			influences[i][j] = [influence_list[j][0], 0]
		else:
			influences[i][j] = [influence_list[j][0], (influence_list[j][1] / influence_sum)]

print(influences[33772])

# Apply cluster transforms to verts.
for i in range(len(influences)):
	influence_list = influences[i]
	vert_pos = mesh.vertices[i]
	vert4 = np.append(vert_pos, 1.0)
	final_vert = np.zeros(4, dtype=np.float64)

	if len(influence_list) == 0:
		final_vert = vert4
		pass
	else:
		# for j in range(min(len(influence_list), 1)):
		for j in range(len(influence_list)):
			# vert4 = vert4 + np.matmul(cluster_transforms[influence_list[j][0]], vert4) * influence_list[j][1]
			final_vert = final_vert + np.matmul(cluster_transforms[influence_list[j][0]], vert4) * influence_list[j][1]
			# final_vert = final_vert + (np.matmul(cluster_transforms[influence_list[j][0]], vert4) - vert4) * influence_list[j][1]

	mesh.vertices[i] = final_vert[:3]
	# mesh.vertices[i] = vert_pos + final_vert[:3]

mesh.compute_vertex_normals()

o3d.visualization.draw_geometries([target_down, mesh, seed_pcl])