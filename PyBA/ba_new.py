# Starting point: https://scipy-cookbook.readthedocs.io/items/bundle_adjustment.html

from __future__ import print_function

import os
import sys
import numpy as np
import time
from scipy.optimize import least_squares
from scipy.sparse import lil_matrix
import matplotlib.pyplot as plt
import cv2

#------------------------------------------------------------------------------------------------------------------------
# Read input file.
#------------------------------------------------------------------------------------------------------------------------
def read_input(filePath):
	with open(filePath, "r") as file:
		n_points, n_views = map(int, file.readline().split())

		# Axis parameters.
		axis_x = np.array(file.readline().split(), dtype=float)
		axis_y = np.array(file.readline().split(), dtype=float)
		axis_z = np.array(file.readline().split(), dtype=float)
		axis_dirs = np.array([axis_x, axis_y, axis_z])

		# Intrinsics.
		cam_mat_0 = np.array(file.readline().split(), dtype=float).reshape(3, 3)
		cam_dist_0 = np.array(file.readline().split(), dtype=float)
		cam_intrins = [cam_mat_0, cam_dist_0]

		# Extrinsics.
		cam_extrins = np.array(file.readline().split(), dtype=float)
	
		# 3D cube points.
		points_3d = np.empty(n_points * 3)
		for i in range(n_points):
			points_pos = file.readline().split()
			for j in range(3):
				points_3d[i * 3 + j] = float(points_pos[j])

		points_3d = points_3d.reshape((n_points, -1))

		# Views and observations.
		obs_pose_indices = np.empty(0, dtype=int)
		obs_point_indices = np.empty(0, dtype=int)
		obs_points_2d = np.empty((0, 2))
		pose_positions = np.empty((n_views, 3))

		for i in range(n_views):
			view_data = file.readline().split()
			n_obs = int(view_data[0])
			pose_positions[i] = [float(view_data[1]), float(view_data[2]), float(view_data[3])]

			for j in range(n_obs):
				view_obs = file.readline().split()
				obs_pose_indices = np.append(obs_pose_indices, i)
				obs_point_indices = np.append(obs_point_indices, int(view_obs[0]))
				obs_points_2d = np.append(obs_points_2d, [[float(view_obs[1]), float(view_obs[2])]], axis=0)

	return cam_intrins, cam_extrins, axis_dirs, points_3d, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d

# Rotate points by given Rodrigues rotation vectors.
def rotate(points, rot_vecs):
	theta = np.linalg.norm(rot_vecs, axis=1)[:, np.newaxis]
	
	with np.errstate(invalid='ignore'):
		v = rot_vecs / theta
		v = np.nan_to_num(v)

	dot = np.sum(points * v, axis=1)[:, np.newaxis]
	cos_theta = np.cos(theta)
	sin_theta = np.sin(theta)

	return cos_theta * points + sin_theta * np.cross(v, points) + dot * (1 - cos_theta) * v

# Convert a Rodrigues rotation vector and a translation vector to a 4x4 transformation matrix.
def buildMatFromVecs(rvec, tvec):
	r, _ = cv2.Rodrigues(rvec)

	world_mat = np.empty([4, 4])
	world_mat[0, 0] = r[0, 0]
	world_mat[0, 1] = r[1, 0]
	world_mat[0, 2] = r[2, 0]

	world_mat[1, 0] = r[0, 1]
	world_mat[1, 1] = r[1, 1]
	world_mat[1, 2] = r[2, 1]

	world_mat[2, 0] = r[0, 2]
	world_mat[2, 1] = r[1, 2]
	world_mat[2, 2] = r[2, 2]

	world_mat[3, 0] = tvec[0]
	world_mat[3, 1] = tvec[1]
	world_mat[3, 2] = tvec[2]
	world_mat[3, 3] = 1.0
	
	return world_mat

# Project 3D points to 2D image.
def project(points_3d, axis_dirs, packed_intrins, cam_extrins, pose_positions, obs_pose_indices, obs_point_indices):
	final_obs = np.empty((obs_pose_indices.shape[0], 2))

	# Unpack intrinsics.
	cam_mat = np.array([[packed_intrins[0], 0, packed_intrins[2]], [0, packed_intrins[1], packed_intrins[3]], [0, 0, 1]])
	cam_dist = np.array([packed_intrins[4], packed_intrins[5], packed_intrins[6], packed_intrins[7]])

	# Make sure the axis directions are normalized.
	norm_axis_x = axis_dirs[0] / np.linalg.norm(axis_dirs[0])
	norm_axis_y = axis_dirs[1] / np.linalg.norm(axis_dirs[1])
	norm_axis_z = axis_dirs[2] / np.linalg.norm(axis_dirs[2])
	
	# Build the transform for each pose.
	pose_translations = np.empty((pose_positions.shape[0], 3))

	for i in range(pose_positions.shape[0]):
		offset_x = pose_positions[i, 0]
		offset_y = pose_positions[i, 1]
		offset_z = pose_positions[i, 2]

		pose_translations[i, 0] = offset_x * norm_axis_x[0] + offset_y * norm_axis_y[0] + offset_z * -norm_axis_z[0]
		pose_translations[i, 1] = offset_x * norm_axis_x[1] + offset_y * norm_axis_y[1] + offset_z * -norm_axis_z[1]
		pose_translations[i, 2] = offset_x * norm_axis_x[2] + offset_y * norm_axis_y[2] + offset_z * -norm_axis_z[2]

	# For each observation point
	for i in range(obs_pose_indices.shape[0]):
		pose_index = obs_pose_indices[i]
		pose_translation = pose_translations[pose_index]
		point_index = obs_point_indices[i]
		point = np.copy(points_3d[point_index])

		# Transform the point.
		point += pose_translation
		
		# Project the point.
		points_2d, _ = cv2.projectPoints(point, cam_extrins[:3], cam_extrins[3:], cam_mat, cam_dist)
		points_2d = points_2d.reshape((-1, 2))
		
		final_obs[i] = points_2d

	return final_obs

# Compute new residuals.
def residuals(params, n_points, pose_positions, obs_pose_indices, obs_point_indices, points_2d):
	axis_dirs = params[:9].reshape((3, 3))
	points_3d = params[9: 9 + n_points * 3].reshape((n_points, 3))
	cam_intrins = params[9 + n_points * 3: 9 + n_points * 3 + 8]
	cam_extrins = params[9 + n_points * 3 + 8: 9 + n_points * 3 + 8 + 6]
	
	proj_p = project(points_3d, axis_dirs, cam_intrins, cam_extrins, pose_positions, obs_pose_indices, obs_point_indices)
	
	return (proj_p - points_2d).ravel()

def bundle_adjustment_sparsity(n_points, n_observations, obs_point_indices):
	m = n_observations * 2
	n = 9 + n_points * 3 + 8 + 6
	A = lil_matrix((m, n), dtype=int)

	i = np.arange(n_observations)

	# Axis directions.
	for s in range(9):
		A[2 * i, s] = 1
		A[2 * i + 1, s] = 1

	# 3D cube points.
	for s in range(3):
		A[2 * i, 9 + obs_point_indices * 3 + s] = 1
		A[2 * i + 1, 9 + obs_point_indices * 3 + s] = 1

	# Camera intrins/extrins.
	for s in range(8 + 6):
		A[2 * i, 9 + n_points * 3 + s] = 1
		A[2 * i + 1, 9 + n_points * 3 + s] = 1

	return A

#------------------------------------------------------------------------------------------------------------------------
# Main.
#------------------------------------------------------------------------------------------------------------------------
# Disable scientific notation for clarity.
np.set_printoptions(suppress = True)

print("Starting bundle adjustment...")

# Print all arguments
print("Arguments:")
for i in range(len(sys.argv)):
	print("  {}: {}".format(i, sys.argv[i]))

show_debug_plots = True

# read_input(sys.argv[1])
cam_intrins, cam_extrins, axis_dirs, points_3d, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d = read_input("new_ba_input.txt")

cam_mat_0 = cam_intrins[0]
cam_dist_0 = cam_intrins[1]
packed_intrins_0 = np.array([cam_mat_0[0, 0], cam_mat_0[1, 1], cam_mat_0[0, 2], cam_mat_0[1, 2], cam_dist_0[0], cam_dist_0[1], cam_dist_0[2], cam_dist_0[3]])

n_views = pose_positions.shape[0]
n_points = points_3d.shape[0]
n_observations = obs_points_2d.shape[0]

print("Poses: {}".format(n_views))
print("Cube points: {}".format(n_points))
print("Observations: {}".format(n_observations))

all_p = project(points_3d, axis_dirs, packed_intrins_0, cam_extrins, pose_positions, obs_pose_indices, obs_point_indices)
					
# plt.plot(obs_points_2d[:,0], obs_points_2d[:,1], 'bo', markersize=1)
# plt.plot(all_p[:,0], all_p[:,1], 'ro', markersize=1)

# plt.xlim([0, 3280])
# plt.ylim([2464, 0])
# plt.show()

x0 = np.hstack((axis_dirs.ravel(), points_3d.ravel(), packed_intrins_0.ravel(), cam_extrins.ravel()))
f0 = residuals(x0, n_points, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d)

# plt.plot(f0, 'bo', markersize=2)
# plt.show()

A = bundle_adjustment_sparsity(n_points, n_observations, obs_point_indices)
# plt.figure()
# plt.spy(A, markersize=1, aspect='auto')
# plt.show()

# x0 = np.hstack((view_params.ravel(), points_3d.ravel(), packed_intrins_0.ravel(), packed_intrins_1.ravel(), relative_pose))
# f0 = residuals(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
# A = bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices)

#------------------------------------------------------------------------------------------------------------------------
# Plot initial points and cameras.
#------------------------------------------------------------------------------------------------------------------------
if show_debug_plots:
	# Print Jacobian sparsity pattern.
	plt.figure()
	plt.spy(A, markersize=1, aspect='auto')
	plt.show()

	all_p = project(points_3d, axis_dirs, packed_intrins_0, cam_extrins, pose_positions, obs_pose_indices, obs_point_indices)

	plt.plot(obs_points_2d[:,0], obs_points_2d[:,1], 'bo', markersize=1)
	plt.plot(all_p[:,0], all_p[:,1], 'ro', markersize=1)
	
	plt.xlim([0, 3280])
	plt.ylim([2464, 0])
	plt.show()

	# Original base pose guess.
	# fig = plt.figure()
	# ax = fig.add_subplot(111, projection='3d')
	# ax.scatter(view_params[:, 3], view_params[:, 4], view_params[:, 5], c='b', marker='o')
	# ax.set_aspect('equal')
	# plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Optimize.
#------------------------------------------------------------------------------------------------------------------------
t0 = time.time()
res = least_squares(residuals, x0, jac_sparsity=A, verbose=2, x_scale='jac', ftol=1e-6, method='trf', args=(n_points, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d))
# res = least_squares(residuals, x0, jac_sparsity=A, verbose=2, max_nfev=60, xtol=1e-10, loss='soft_l1', f_scale=1, method='trf', args=(n_points, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d))
# least_squares(residuals, x0, verbose=2, method ='trf', xtol=1e-10, loss='soft_l1', f_scale=0.1, args=(obj_pts, left_pts, right_pts))
t1 = time.time()

#------------------------------------------------------------------------------------------------------------------------
# Print results.
#------------------------------------------------------------------------------------------------------------------------
print("Optimization took {0:.3f} seconds".format(t1 - t0))

# Calculate reprojection error.
f0 = residuals(x0, n_points, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d)
f1 = residuals(res.x, n_points, pose_positions, obs_pose_indices, obs_point_indices, obs_points_2d)

# Gather optimized parameters.
axis_dirs_optimized = res.x[:9].reshape((3, 3))
points_3d_optimized = res.x[9: 9 + n_points * 3].reshape((n_points, 3))
cam_intrinsics_optimized = res.x[9 + n_points * 3: 9 + n_points * 3 + 8]
cam_extrins_optimized = res.x[9 + n_points * 3 + 8: 9 + n_points * 3 + 8 + 6]

# Normalize axis directions.
axis_dirs_optimized[0] /= np.linalg.norm(axis_dirs_optimized[0])
axis_dirs_optimized[1] /= np.linalg.norm(axis_dirs_optimized[1])
axis_dirs_optimized[2] /= np.linalg.norm(axis_dirs_optimized[2])

print("Initial RMSE: {}".format(np.sqrt(np.mean(f0**2))))
print("Final RMSE: {}".format(np.sqrt(np.mean(f1**2))))

print("Initial axis dirs: {}".format(axis_dirs))
print("Final axis dirs: {}".format(axis_dirs_optimized))

print("Initial intrinsics: {}".format(packed_intrins_0))
print("Final intrinsics: {}".format(cam_intrinsics_optimized))

print("Initial cam extrins: {}".format(cam_extrins))
print("Final cam extrins: {}".format(cam_extrins_optimized))

if show_debug_plots:
	# Plot residuals.
	plt.plot(f0, 'bo', markersize=2)
	plt.plot(f1, 'ro', markersize=2)
	plt.show()

	all_p = project(points_3d_optimized, axis_dirs_optimized, cam_intrinsics_optimized, cam_extrins_optimized, pose_positions, obs_pose_indices, obs_point_indices)

	plt.plot(obs_points_2d[:,0], obs_points_2d[:,1], 'bo', markersize=2)
	plt.plot(all_p[:,0], all_p[:,1], 'ro', markersize=2)
	plt.xlim([0, 3280])
	plt.ylim([2464, 0])
	plt.show()

	# Plot 3D points after optimization.
	fig = plt.figure()
	ax = fig.add_subplot(111, projection='3d')
	ax.scatter(points_3d[:, 0], points_3d[:, 1], points_3d[:, 2], c='b', marker='o')
	ax.scatter(points_3d_optimized[:, 0], points_3d_optimized[:, 1], points_3d_optimized[:, 2], c='r', marker='o')
	ax.set_aspect('equal')
	plt.show()

	# Plot all cubes with optimized model.
	# fig = plt.figure()
	# ax = fig.add_subplot(111, projection='3d')
	
	# norm_axis_x = axis_dirs_optimized[0]
	# norm_axis_y = axis_dirs_optimized[1]
	# norm_axis_z = axis_dirs_optimized[2]

	# for i in range(n_views):
	# 	# Build the transform for each pose.
	# 	offset_x = pose_positions[i, 0]
	# 	offset_y = pose_positions[i, 1]
	# 	offset_z = pose_positions[i, 2]

	# 	pose_translation = np.empty(3)
	# 	pose_translation[0] = offset_x * norm_axis_x[0] + offset_y * norm_axis_y[0] + offset_z * -norm_axis_z[0]
	# 	pose_translation[1] = offset_x * norm_axis_x[1] + offset_y * norm_axis_y[1] + offset_z * -norm_axis_z[1]
	# 	pose_translation[2] = offset_x * norm_axis_x[2] + offset_y * norm_axis_y[2] + offset_z * -norm_axis_z[2]

	# 	# Transform the point.
	# 	points_3d_transformed = points_3d_optimized + pose_translation

	# 	# Project the point.
	# 	ax.scatter(points_3d_transformed[:, 0], points_3d_transformed[:, 1], points_3d_transformed[:, 2], c='r', marker='o')

	# ax.set_aspect('equal')
	# plt.show()

	# Plot optimized points and cameras.
	# fig = plt.figure()
	# ax = fig.add_subplot(111, projection='3d')
	# ax.scatter(view_params_optimized[:, 3], view_params_optimized[:, 4], view_params_optimized[:, 5], c='r', marker='o')
	# ax.set_aspect('equal')
	# plt.show()

	# Plot rotation vectors for each view.
	# plt.plot(view_params_optimized[:, 0], 'ro', markersize=1)
	# plt.plot(view_params_optimized[:, 1], 'go', markersize=1)
	# plt.plot(view_params_optimized[:, 2], 'bo', markersize=1)
	# plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Save results.
#------------------------------------------------------------------------------------------------------------------------
# with open(sys.argv[2], "w") as file:
with open("new_ba_output.txt", "w") as file:
	file.write("{}\n".format( n_points))

	# Camera intrinsics
	file.write("{} {} {} {}\n".format(cam_intrinsics_optimized[0], cam_intrinsics_optimized[1], cam_intrinsics_optimized[2], cam_intrinsics_optimized[3]))
	file.write("{} {} {} {}\n".format(cam_intrinsics_optimized[4], cam_intrinsics_optimized[5], cam_intrinsics_optimized[6], cam_intrinsics_optimized[7]))

	# Camera extrinsics
	file.write("{} {} {} {} {} {}\n".format(cam_extrins_optimized[0], cam_extrins_optimized[1], cam_extrins_optimized[2], cam_extrins_optimized[3], cam_extrins_optimized[4], cam_extrins_optimized[5]))

	# Axis directions.
	for i in range(3):
		file.write("{} {} {}\n".format(axis_dirs_optimized[i, 0], axis_dirs_optimized[i, 1], axis_dirs_optimized[i, 2]))

	# Cube points.
	for i in range(n_points):
		file.write("{} {} {} {}\n".format(i, points_3d_optimized[i, 0], points_3d_optimized[i, 1], points_3d_optimized[i, 2]))