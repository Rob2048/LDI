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
		n_views, n_points, n_observations = map(int, file.readline().split())

		# Intrinsics
		cam_mat_0 = np.array(file.readline().split(), dtype=float).reshape(3, 3)
		cam_dist_0 = np.array(file.readline().split(), dtype=float)

		cam_mat_1 = np.array(file.readline().split(), dtype=float).reshape(3, 3)
		cam_dist_1 = np.array(file.readline().split(), dtype=float)

		cam_intrins = [cam_mat_0, cam_dist_0, cam_mat_1, cam_dist_1]
		
		relative_pose = np.empty(6)
		relative_pose_params = file.readline().split()
		for i in range(6):
			relative_pose[i] = float(relative_pose_params[i])

		view_indices = np.empty(n_observations, dtype=int)
		cam_indices = np.empty(n_observations, dtype=int)
		point_indices = np.empty(n_observations, dtype=int)
		points_2d = np.empty((n_observations, 2))

		for i in range(n_observations):
			view_index, cam_index, point_index, x, y = file.readline().split()
			view_indices[i] = int(view_index)
			cam_indices[i] = int(cam_index)
			point_indices[i] = int(point_index)
			points_2d[i] = [float(x), float(y)]

		view_params = np.empty(n_views * 6)
		view_sample_inds = np.empty(n_views, dtype=int)
		for v in range(n_views):
			view_data = file.readline().split()
			
			view_sample_inds[v] = int(view_data[0])
			
			for i in range(6):
				view_params[v * 6 + i] = float(view_data[i + 1])
			
		view_params = view_params.reshape((n_views, -1))

		points_3d = np.empty(n_points * 3)
		for i in range(n_points):
			points_pos = file.readline().split()
			for j in range(3):
				points_3d[i * 3 + j] = float(points_pos[j])

		points_3d = points_3d.reshape((n_points, -1))

	return cam_intrins, relative_pose, view_sample_inds, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d

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
def project(points_3d, view_params, point_indices, cam_indices, view_indices, packed_intrins_0, packed_intrins_1, relative_pose):
	final_obs = np.empty((0, 2))
	
	cam_0_indices = np.where(cam_indices == 0)[0]
	cam_1_indices = np.where(cam_indices == 1)[0]

	# Unpack intrinsics.
	cam_mat_0 = np.array([[packed_intrins_0[0], 0, packed_intrins_0[2]], [0, packed_intrins_0[1], packed_intrins_0[3]], [0, 0, 1]])
	cam_dist_0 = np.array([packed_intrins_0[4], packed_intrins_0[5], packed_intrins_0[6], packed_intrins_0[7]])

	cam_mat_1 = np.array([[packed_intrins_1[0], 0, packed_intrins_1[2]], [0, packed_intrins_1[1], packed_intrins_1[3]], [0, 0, 1]])
	cam_dist_1 = np.array([packed_intrins_1[4], packed_intrins_1[5], packed_intrins_1[6], packed_intrins_1[7]])
	
	# For each base pose view
	for i in range(view_params.shape[0]):
		# Get points in this view for camera 0.
		view_a = np.where(view_indices == i)[0]
		view_cam_a = np.intersect1d(view_a, cam_0_indices)
		points_a = points_3d[point_indices[view_cam_a]]

		points_2d_0, _ = cv2.projectPoints(points_a, view_params[i, :3], view_params[i, 3:], cam_mat_0, cam_dist_0)
		points_2d_0 = points_2d_0.reshape((-1, 2))
		
		final_obs = np.append(final_obs, points_2d_0, axis=0)

		# Get points in this view for camera 1.
		view_cam_a = np.intersect1d(view_a, cam_1_indices)
		points_a = points_3d[point_indices[view_cam_a]]

		# Transform point based on base pose.
		points_a = rotate(points_a, np.tile(view_params[i, :3], (points_a.shape[0], 1)))
		points_a += view_params[i, 3:6]

		points_2d_0, _ = cv2.projectPoints(points_a, relative_pose[:3], relative_pose[3:], cam_mat_1, cam_dist_1)
		points_2d_0 = points_2d_0.reshape((-1, 2))
		
		final_obs = np.append(final_obs, points_2d_0, axis=0)

	return final_obs

# Compute residuals.
def residuals(params, n_views, n_points, cam_indices, view_indices, point_indices, points_2d):
	view_params = params[:n_views * 6].reshape((n_views, 6))
	points_3d = params[n_views * 6: + n_views * 6 + n_points * 3].reshape((n_points, 3))
	cam_intrins_0 = params[n_views * 6 + n_points * 3:n_views * 6 + n_points * 3 + 8]
	cam_intrins_1 = params[n_views * 6 + n_points * 3 + 8:n_views * 6 + n_points * 3 + 8 + 8]
	rel_pose = params[n_views * 6 + n_points * 3 + 8 + 8:n_views * 6 + n_points * 3 + 8 + 8 + 6]
	
	proj_p = project(points_3d, view_params, point_indices, cam_indices, view_indices, cam_intrins_0, cam_intrins_1, rel_pose)
	
	return (proj_p - points_2d).ravel()

def bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices):
	m = view_indices.size * 2
	n = n_views * 6 + n_points * 3 + 8 + 8 + 6
	A = lil_matrix((m, n), dtype=int)

	i = np.arange(view_indices.size)

	# Base poses.
	for s in range(6):
		A[2 * i, view_indices * 6 + s] = 1
		A[2 * i + 1, view_indices * 6 + s] = 1

	# 3D cube points.
	for s in range(3):
		A[2 * i, n_views * 6 + point_indices * 3 + s] = 1
		A[2 * i + 1, n_views * 6 + point_indices * 3 + s] = 1

	# Camera intrinsics.
	for s in range(8):
		A[2 * i, n_views * 6 + n_points * 3 + cam_indices * 8 + s] = 1
		A[2 * i + 1, n_views * 6 + n_points * 3 + cam_indices * 8 + s] = 1

	# Relative stereo pose.
	i = np.where(cam_indices == 1)[0]
	for s in range(6):
		A[2 * i, n_views * 6 + n_points * 3 + 8 + 8 + s] = 1
		A[2 * i + 1, n_views * 6 + n_points * 3 + 8 + 8 + s] = 1

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

show_debug_plots = False

cam_intrins, relative_pose, view_sample_inds, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d = read_input(sys.argv[1])

cam_mat_0 = cam_intrins[0]
cam_dist_0 = cam_intrins[1]
cam_mat_1 = cam_intrins[2]
cam_dist_1 = cam_intrins[3]

packed_intrins_0 = np.array([cam_mat_0[0, 0], cam_mat_0[1, 1], cam_mat_0[0, 2], cam_mat_0[1, 2], cam_dist_0[0], cam_dist_0[1], cam_dist_0[2], cam_dist_0[3]])
packed_intrins_1 = np.array([cam_mat_1[0, 0], cam_mat_1[1, 1], cam_mat_1[0, 2], cam_mat_1[1, 2], cam_dist_1[0], cam_dist_1[1], cam_dist_1[2], cam_dist_1[3]])

n_views = view_params.shape[0]
n_points = points_3d.shape[0]

print("(all images) n_views: {}".format(n_views))
print("(3D points) n_points: {}".format(n_points))
print("Observations: {}".format(points_2d.shape[0]))

x0 = np.hstack((view_params.ravel(), points_3d.ravel(), packed_intrins_0.ravel(), packed_intrins_1.ravel(), relative_pose))
f0 = residuals(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
A = bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices)

#------------------------------------------------------------------------------------------------------------------------
# Plot initial points and cameras.
#------------------------------------------------------------------------------------------------------------------------
if show_debug_plots:
	# Print Jacobian sparsity pattern.
	plt.figure()
	plt.spy(A, markersize=1, aspect='auto')
	plt.show()

	all_p = project(points_3d, view_params, point_indices, cam_indices, view_indices, packed_intrins_0, packed_intrins_1, relative_pose)

	plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=1)
	plt.plot(all_p[:,0], all_p[:,1], 'ro', markersize=1)
	
	plt.xlim([0, 3280])
	plt.ylim([2464, 0])
	plt.show()

	# Original base pose guess.
	fig = plt.figure()
	ax = fig.add_subplot(111, projection='3d')
	ax.scatter(view_params[:, 3], view_params[:, 4], view_params[:, 5], c='b', marker='o')
	ax.set_aspect('equal')
	plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Optimize.
#------------------------------------------------------------------------------------------------------------------------
t0 = time.time()
res = least_squares(residuals, x0, jac_sparsity=A, verbose=2, x_scale='jac', ftol=1e-6, method='trf', args=(n_views, n_points, cam_indices, view_indices, point_indices, points_2d))
# res = least_squares(residuals, x0, jac_sparsity=A, verbose=2, max_nfev=30, xtol=1e-10, loss='soft_l1', f_scale=0.1, method='trf', args=(n_views, n_points, cam_indices, view_indices, point_indices, points_2d))
# least_squares(residuals, x0, verbose=2, method ='trf', xtol=1e-10, loss='soft_l1', f_scale=0.1, args=(obj_pts, left_pts, right_pts))
t1 = time.time()

#------------------------------------------------------------------------------------------------------------------------
# Print results.
#------------------------------------------------------------------------------------------------------------------------
print("Optimization took {0:.3f} seconds".format(t1 - t0))

# Calculate reprojection error.
f0 = residuals(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
f1 = residuals(res.x, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)

# Gather optimized parameters.
view_params_optimized = res.x[:n_views * 6].reshape((n_views, 6))
points_3d_optimized = res.x[n_views * 6:n_views * 6 + n_points * 3].reshape((n_points, 3))
cam_intrinsics_optimized = res.x[n_views * 6 + n_points * 3:n_views * 6 + n_points * 3 + 8 + 8].reshape((2, 8))
relative_pose_optimized = res.x[n_views * 6 + n_points * 3 + 8 + 8:n_views * 6 + n_points * 3 + 8 + 8 + 6]
relative_params_optimized = np.array([[0, 0, 0, 0, 0, 0], relative_pose_optimized])

print("Initial RMSE: {}".format(np.sqrt(np.mean(f0**2))))
print("Final RMSE: {}".format(np.sqrt(np.mean(f1**2))))
# print("Initial intrinsics: {}".format(cam_intrins))
print("Final intrinsics: {}".format(cam_intrinsics_optimized))
print("Initial relative pose: {}".format(relative_pose))
print("Final relative pose: {}".format(relative_pose_optimized))

if show_debug_plots:
	# Plot residuals.
	plt.plot(f0, 'bo', markersize=2)
	plt.plot(f1, 'ro', markersize=2)
	plt.show()

	all_p = project(points_3d_optimized, view_params_optimized, point_indices, cam_indices, view_indices, cam_intrinsics_optimized[0], cam_intrinsics_optimized[1], relative_pose_optimized)

	plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
	plt.plot(all_p[:,0], all_p[:,1], 'ro', markersize=2)
	plt.show()

	# Plot 3D points after optimization.
	fig = plt.figure()
	ax = fig.add_subplot(111, projection='3d')
	ax.scatter(points_3d[:, 0], points_3d[:, 1], points_3d[:, 2], c='b', marker='o')
	ax.scatter(points_3d_optimized[:, 0], points_3d_optimized[:, 1], points_3d_optimized[:, 2], c='r', marker='o')
	ax.set_aspect('equal')
	plt.show()

	# Plot optimized points and cameras.
	fig = plt.figure()
	ax = fig.add_subplot(111, projection='3d')
	ax.scatter(view_params_optimized[:, 3], view_params_optimized[:, 4], view_params_optimized[:, 5], c='r', marker='o')
	ax.set_aspect('equal')
	plt.show()

	# Plot rotation vectors for each view.
	# plt.plot(view_params_optimized[:, 0], 'ro', markersize=1)
	# plt.plot(view_params_optimized[:, 1], 'go', markersize=1)
	# plt.plot(view_params_optimized[:, 2], 'bo', markersize=1)
	# plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Save results.
#------------------------------------------------------------------------------------------------------------------------
with open(sys.argv[2], "w") as file:
	file.write("{} {}\n".format(n_views, n_points))

	# camera intrinsics
	for i in range(2):
		file.write("{} {} {} {}\n".format(cam_intrinsics_optimized[i, 0], cam_intrinsics_optimized[i, 1], cam_intrinsics_optimized[i, 2], cam_intrinsics_optimized[i, 3]))
		file.write("{} {} {} {}\n".format(cam_intrinsics_optimized[i, 4], cam_intrinsics_optimized[i, 5], cam_intrinsics_optimized[i, 6], cam_intrinsics_optimized[i, 7]))

	file.write("{} {} {} {} {} {}\n".format(relative_pose_optimized[0], relative_pose_optimized[1], relative_pose_optimized[2], relative_pose_optimized[3], relative_pose_optimized[4], relative_pose_optimized[5]))

	for i in range(n_views):
		file.write("{} {} {} {} {} {} {}\n".format(view_sample_inds[i], view_params_optimized[i, 0], view_params_optimized[i, 1], view_params_optimized[i, 2], view_params_optimized[i, 3], view_params_optimized[i, 4], view_params_optimized[i, 5]))

	for i in range(n_points):
		file.write("{} {} {} {}\n".format(i, points_3d_optimized[i, 0], points_3d_optimized[i, 1], points_3d_optimized[i, 2]))