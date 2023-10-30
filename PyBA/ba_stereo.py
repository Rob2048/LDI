# Inspired by https://scipy-cookbook.readthedocs.io/items/bundle_adjustment.html

from __future__ import print_function

import os
import numpy as np
import time
from scipy.optimize import least_squares
from scipy.sparse import lil_matrix
import matplotlib.pyplot as plt
import cv2

#------------------------------------------------------------------------------------------------------------------------
# Read input file.
#------------------------------------------------------------------------------------------------------------------------
def read_input():
	with open("ba_input.txt", "r") as file:
		n_views, n_points, n_observations = map(int, file.readline().split())

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

	return relative_pose, view_sample_inds, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d

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
def project(points, view_params, cam_intrins, relative_params):
	base_pose_r = view_params[:, :3]
	base_pose_t = view_params[:, 3:6]

	points_proj = rotate(points, base_pose_r)
	points_proj += base_pose_t

	realtive_pose_r = relative_params[:, :3]
	realtive_pose_t = relative_params[:, 3:6]

	points_proj = rotate(points_proj, realtive_pose_r)
	points_proj += realtive_pose_t

	points_proj = -points_proj[:, :2] / points_proj[:, 2, np.newaxis]
	
	f = cam_intrins[:, 0]	
	k1 = cam_intrins[:, 1]
	k2 = cam_intrins[:, 2]

	n = np.sum(points_proj**2, axis=1)
	r = 1 + k1 * n + k2 * n**2

	points_proj *= (r * f)[:, np.newaxis]

	return points_proj

# Compute residuals.
def fun(params, n_views, n_points, cam_indices, view_indices, point_indices, points_2d):
	view_params = params[:n_views * 6].reshape((n_views, 6))
	points_3d = params[n_views * 6: + n_views * 6 + n_points * 3].reshape((n_points, 3))
	cam_intrins = params[n_views * 6 + n_points * 3:n_views * 6 + n_points * 3 + 6].reshape((2, 3))

	rel_pose = params[n_views * 6 + n_points * 3 + 6:n_views * 6 + n_points * 3 + 6 + 6]
	relative_params = np.array([[0, 0, 0, 0, 0, 0], rel_pose])
	
	points_proj = project(points_3d[point_indices], view_params[view_indices], cam_intrins[cam_indices], relative_params[cam_indices])

	return (points_proj - points_2d).ravel()

def bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices):
	m = view_indices.size * 2
	n = n_views * 6 + n_points * 3 + 6 + 6
	A = lil_matrix((m, n), dtype=int)

	i = np.arange(view_indices.size)

	# Base poses.
	for s in range(6):
		A[2 * i, view_indices * 6 + s] = 1
		A[2 * i + 1, view_indices * 6 + s] = 1

	# Cube points.
	for s in range(3):
		A[2 * i, n_views * 6 + point_indices * 3 + s] = 1
		A[2 * i + 1, n_views * 6 + point_indices * 3 + s] = 1

	# Camera intrinsics.
	for s in range(3):
		A[2 * i, n_views * 6 + n_points * 3 + cam_indices * 3 + s] = 1
		A[2 * i + 1, n_views * 6 + n_points * 3 + cam_indices * 3 + s] = 1

	# Relative stereo pose.
	i = np.where(cam_indices == 1)[0]
	for s in range(6):
		A[2 * i, n_views * 6 + n_points * 3 + 6 + s] = 1
		A[2 * i + 1, n_views * 6 + n_points * 3 + 6 + s] = 1

	return A

#------------------------------------------------------------------------------------------------------------------------
# Main.
#------------------------------------------------------------------------------------------------------------------------
# Disable scientific notation for clarity.
np.set_printoptions(suppress = True)

show_debug_plots = True

relative_pose, view_sample_inds, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d = read_input()

n_views = view_params.shape[0]
n_points = points_3d.shape[0]

cam_intrinsics = np.array([[2600, 0, 0], [2600, 0, 0]])
relative_params = np.array([[0, 0, 0, 0, 0, 0], relative_pose])

print("(all images) n_views: {}".format(n_views))
print("(3D points) n_points: {}".format(n_points))
print("Observations: {}".format(points_2d.shape[0]))

x0 = np.hstack((view_params.ravel(), points_3d.ravel(), cam_intrinsics.ravel(), relative_pose))
f0 = fun(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
A = bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices)

#------------------------------------------------------------------------------------------------------------------------
# Plot initial points and cameras.
#------------------------------------------------------------------------------------------------------------------------
if show_debug_plots:
	# Print Jacobian sparsity pattern.
	plt.figure()
	plt.spy(A, markersize=1, aspect='auto')
	plt.show()

	cam_0_indices = np.where(cam_indices == 0)[0]
	cam_1_indices = np.where(cam_indices == 1)[0]

	point_indices_0 = point_indices[cam_0_indices]
	view_indices_0 = view_indices[cam_0_indices]
	cam_indices_0 = cam_indices[cam_0_indices]

	point_indices_1 = point_indices[cam_1_indices]
	view_indices_1 = view_indices[cam_1_indices]
	cam_indices_1 = cam_indices[cam_1_indices]

	cam_0_proj_points = project(points_3d[point_indices_0], view_params[view_indices_0], cam_intrinsics[cam_indices_0], relative_params[cam_indices_0])
	cam_1_proj_points = project(points_3d[point_indices_1], view_params[view_indices_1], cam_intrinsics[cam_indices_1], relative_params[cam_indices_1])
	cam_proj_points = project(points_3d[point_indices], view_params[view_indices], cam_intrinsics[cam_indices], relative_params[cam_indices])

	plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=1)
	# plt.plot(cam_proj_points[:,0], cam_proj_points[:,1], 'ro', markersize=1)
	plt.plot(cam_0_proj_points[:,0], cam_0_proj_points[:,1], 'ro', markersize=1)
	plt.plot(cam_1_proj_points[:,0], cam_1_proj_points[:,1], 'go', markersize=1)

	plt.xlim([-1640, 1640])
	plt.ylim([-1232, 1232])
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
res = least_squares(fun, x0, jac_sparsity=A, verbose=2, x_scale='jac', ftol=1e-8, method='trf', args=(n_views, n_points, cam_indices, view_indices, point_indices, points_2d))
# res = least_squares(fun, x0, jac_sparsity=A, verbose=2, max_nfev=400, xtol=1e-10, loss='soft_l1', f_scale=0.1, method='trf', args=(n_views, n_points, cam_indices, view_indices, point_indices, points_2d))
# least_squares(fun, x0, verbose=2, method ='trf', xtol=1e-10,
#                            loss='soft_l1', f_scale=0.1,
#                            args=(obj_pts, left_pts, right_pts))
t1 = time.time()

#------------------------------------------------------------------------------------------------------------------------
# Print results.
#------------------------------------------------------------------------------------------------------------------------
print("Optimization took {0:.3f} seconds".format(t1 - t0))

# Calculate reprojection error.
f0 = fun(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
f1 = fun(res.x, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)

# Gather optimized parameters.
view_params_optimized = res.x[:n_views * 6].reshape((n_views, 6))
points_3d_optimized = res.x[n_views * 6:n_views * 6 + n_points * 3].reshape((n_points, 3))
cam_intrinsics_optimized = res.x[n_views * 6 + n_points * 3:n_views * 6 + n_points * 3 + 6].reshape((2, 3))
relative_pose_optimized = res.x[n_views * 6 + n_points * 3 + 6:n_views * 6 + n_points * 3 + 6 + 6]
relative_params_optimized = np.array([[0, 0, 0, 0, 0, 0], relative_pose_optimized])

print("Initial RMSE: {}".format(np.sqrt(np.mean(f0**2))))
print("Final RMSE: {}".format(np.sqrt(np.mean(f1**2))))
print("Initial intrinsics: {}".format(cam_intrinsics))
print("Final intrinsics: {}".format(cam_intrinsics_optimized))
print("Initial intrinsics: {}".format(relative_pose))
print("Final intrinsics: {}".format(relative_pose_optimized))

if show_debug_plots:
	# Plot residuals.
	plt.plot(f0, 'bo', markersize=2)
	plt.plot(f1, 'ro', markersize=2)
	plt.show()

	# Plot reprojection with optimized params.
	cam_0_indices = np.where(cam_indices == 0)[0]
	point_indices_0 = point_indices[cam_0_indices]
	view_indices_0 = view_indices[cam_0_indices]
	cam_indices_0 = cam_indices[cam_0_indices]

	# points_p = project(points_3d_optimized[point_indices], view_params_optimized[view_indices], cam_intrinsics_optimized[cam_indices], relative_params_optimized[cam_indices])
	points_p = project(points_3d_optimized[point_indices_0], view_params_optimized[view_indices_0], cam_intrinsics_optimized[cam_indices_0], relative_params_optimized[cam_indices_0])

	plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
	plt.plot(points_p[:,0], points_p[:,1], 'ro', markersize=2)
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
	# ax.scatter(view_params[:, 3], view_params[:, 4], view_params[:, 5], c='b', marker='o')
	ax.scatter(view_params_optimized[:, 3], view_params_optimized[:, 4], view_params_optimized[:, 5], c='r', marker='o')
	ax.set_aspect('equal')
	plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Save results.
#------------------------------------------------------------------------------------------------------------------------
with open("ba_result.txt", "w") as file:
	file.write("{} {}\n".format(n_views, n_points))

	# camera intrinsics
	for i in range(2):
		file.write("{} {} {} {}\n".format(i, cam_intrinsics_optimized[i, 0], cam_intrinsics_optimized[i, 1], cam_intrinsics_optimized[i, 2]))

	file.write("{} {} {} {} {} {}\n".format(relative_pose_optimized[0], relative_pose_optimized[1], relative_pose_optimized[2], relative_pose_optimized[3], relative_pose_optimized[4], relative_pose_optimized[5]))

	for i in range(n_views):
		file.write("{} {} {} {} {} {} {}\n".format(view_sample_inds[i], view_params_optimized[i, 0], view_params_optimized[i, 1], view_params_optimized[i, 2], view_params_optimized[i, 3], view_params_optimized[i, 4], view_params_optimized[i, 5]))

	for i in range(n_points):
		file.write("{} {} {} {}\n".format(i, points_3d_optimized[i, 0], points_3d_optimized[i, 1], points_3d_optimized[i, 2]))