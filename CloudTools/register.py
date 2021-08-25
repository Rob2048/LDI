import open3d as o3d
import numpy as np
import copy
import matplotlib.pyplot as plt
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

def apply_registration_result(source, target, transformation):
	source.paint_uniform_color([0.07058821, 0.7273544, 1.0])
	target.paint_uniform_color([0.6, 0.2, 0.65])
	source.transform(transformation)

def draw_registration_result_temp(source, target, transformation):
	source_temp = copy.deepcopy(source)
	target_temp = copy.deepcopy(target)
	source_temp.paint_uniform_color([0.07058821, 0.7273544, 1.0])
	target_temp.paint_uniform_color([0.6, 0.2, 0.65])
	source_temp.transform(transformation)
	# target_temp.translate((3, 0, 0))
	o3d.visualization.draw_geometries([source_temp, target_temp])

def preprocess_point_cloud(pcd, voxel_size):
	print(":: Downsample with a voxel size %.3f." % voxel_size)
	pcd_down = pcd.voxel_down_sample(voxel_size)

	radius_normal = voxel_size * 2
	print(":: Estimate normal with search radius %.3f." % radius_normal)
	pcd_down.estimate_normals(
		o3d.geometry.KDTreeSearchParamHybrid(radius=radius_normal, max_nn=30))

	radius_feature = voxel_size * 5
	print(":: Compute FPFH feature with search radius %.3f." % radius_feature)
	pcd_fpfh = o3d.pipelines.registration.compute_fpfh_feature(
		pcd_down,
		o3d.geometry.KDTreeSearchParamHybrid(radius=radius_feature, max_nn=100))
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
			o3d.pipelines.registration.CorrespondenceCheckerBasedOnEdgeLength(
				0.9),
			o3d.pipelines.registration.CorrespondenceCheckerBasedOnDistance(
				distance_threshold)
		], o3d.pipelines.registration.RANSACConvergenceCriteria(100000, 0.999))
	return result

#----------------------------------------------------------------------------------------------------------------------------------
# Triangulation.
#----------------------------------------------------------------------------------------------------------------------------------
# #radii = [0.005, 0.01, 0.02, 0.04]
#radii = [0.05, 0.1, 0.2, 0.4]
#rec_mesh = o3d.geometry.TriangleMesh.create_from_point_cloud_ball_pivoting(downpcd, o3d.utility.DoubleVector(radii))
#o3d.visualization.draw_geometries([rec_mesh])


#----------------------------------------------------------------------------------------------------------------------------------
# Load and prepare data.
#----------------------------------------------------------------------------------------------------------------------------------
print("Point cloud test")

# Source mesh.
mesh = o3d.io.read_triangle_mesh("modelFull.stl")
mesh.compute_vertex_normals()
mesh.paint_uniform_color([0.07058821, 0.7273544, 1.0])

R = mesh.get_rotation_matrix_from_xyz((-np.pi / 2, 0, np.pi * 0))
mesh.rotate(R, center=(0, 0, 0))
mesh.scale(0.049, center=(0, 0, 0))
# mesh.scale(0.0, center=(0, 0, 0))

meshCloud = mesh.sample_points_poisson_disk(number_of_points=20000, init_factor=5)

# Target mesh (scan).
pcd = o3d.io.read_point_cloud("scanPoints.ply")
print(pcd)

cl, ind = pcd.remove_statistical_outlier(nb_neighbors=20, std_ratio=2.0)
inlier_cloud = pcd.select_by_index(ind)
#display_inlier_outlier(pcd, ind)

# downpcd = inlier_cloud.voxel_down_sample(voxel_size=0.01)
downpcd = pcd.voxel_down_sample(voxel_size=0.01)
downpcd.paint_uniform_color([0.6, 0.2, 0.65])
# downpcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=0.1, max_nn=30))
downpcd.estimate_normals(search_param=o3d.geometry.KDTreeSearchParamHybrid(radius=0.05, max_nn=200))
print(downpcd)
o3d.visualization.draw_geometries([downpcd])

#----------------------------------------------------------------------------------------------------------------------------------
# Global registration.
#----------------------------------------------------------------------------------------------------------------------------------

source, target, source_down, target_down, source_fpfh, target_fpfh = prepare_dataset(0.05, meshCloud, downpcd)

result_ransac = execute_global_registration(source_down, target_down,
                                            source_fpfh, target_fpfh,
                                            0.05)

# o3d.visualization.draw_geometries([downpcd, meshCloud])

#apply_registration_result(source_down, target_down, result_ransac.transformation)
#o3d.visualization.draw_geometries([source_down, target_down])

#----------------------------------------------------------------------------------------------------------------------------------
# Local registration.
#----------------------------------------------------------------------------------------------------------------------------------
threshold = 0.02
trans_init = result_ransac.transformation
evaluation = o3d.pipelines.registration.evaluate_registration(source_down, target_down, threshold, trans_init)
print(evaluation)

print("Apply point-to-point ICP")
reg_p2p = o3d.pipelines.registration.registration_icp(
    source_down, target_down, threshold, trans_init,
    o3d.pipelines.registration.TransformationEstimationPointToPoint(with_scaling=True),
	o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=10000))
print(reg_p2p)
print("Transformation is:")
print(reg_p2p.transformation)
draw_registration_result_temp(source_down, target_down, reg_p2p.transformation)