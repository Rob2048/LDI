# https://scipy-cookbook.readthedocs.io/items/bundle_adjustment.html

from __future__ import print_function

import os
import numpy as np
import time
from scipy.optimize import least_squares
from scipy.sparse import lil_matrix
import matplotlib.pyplot as plt

#------------------------------------------------------------------------------------------------------------------------
# Read input file.
#------------------------------------------------------------------------------------------------------------------------
def read_bal_data():
	# with open("test-problem.txt", "r") as file:
	with open("ba_test.txt", "r") as file:
		n_views, n_points, n_observations = map(int, file.readline().split())

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

		camera_ids = np.empty(n_views)
		view_params = np.empty(n_views * 6)
		for c in range(n_views):
			view_data = file.readline().split()
			camera_ids[c] = view_data[1]

			for i in range(6):
				view_params[c * 6 + i] = float(view_data[i + 2])
			
		view_params = view_params.reshape((n_views, -1))

		points_3d = np.empty(n_points * 3)
		for i in range(n_points):
			points_pos = file.readline().split()
			for j in range(3):
				points_3d[i * 3 + j] = float(points_pos[j])

		points_3d = points_3d.reshape((n_points, -1))

	return camera_ids, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d

# Rotate points by given rotation vectors.
def rotate(points, rot_vecs):
	theta = np.linalg.norm(rot_vecs, axis=1)[:, np.newaxis]
	
	with np.errstate(invalid='ignore'):
		v = rot_vecs / theta
		v = np.nan_to_num(v)

	dot = np.sum(points * v, axis=1)[:, np.newaxis]
	cos_theta = np.cos(theta)
	sin_theta = np.sin(theta)

	return cos_theta * points + sin_theta * np.cross(v, points) + dot * (1 - cos_theta) * v

# Project 3D points to 2D image.
def project(points, view_params, cam_intrins):
	points_proj = rotate(points, view_params[:, :3])
	points_proj += view_params[:, 3:6]
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
	
	points_proj = project(points_3d[point_indices], view_params[view_indices], cam_intrins[cam_indices])

	return (points_proj - points_2d).ravel()

def bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices):
	m = view_indices.size * 2
	n = n_views * 6 + n_points * 3 + 6
	A = lil_matrix((m, n), dtype=int)

	i = np.arange(view_indices.size)

	# Poses.
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

	return A

#------------------------------------------------------------------------------------------------------------------------
# Main.
#------------------------------------------------------------------------------------------------------------------------
camera_ids, view_params, points_3d, view_indices, cam_indices, point_indices, points_2d = read_bal_data()

n_views = view_params.shape[0]
n_points = points_3d.shape[0]

cam_intrinsics = np.array([[2600, 0, 0], [2600, 0, 0]])

print("(all images) n_views: {}".format(n_views))
print("(3D points) n_points: {}".format(n_points))
print("Total number of view indices: {}".format(view_indices.shape[0]))

x0 = np.hstack((view_params.ravel(), points_3d.ravel(), cam_intrinsics.ravel()))
f0 = fun(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
A = bundle_adjustment_sparsity(n_views, n_points, view_indices, point_indices, cam_indices)

# Print Jacobian sparsity pattern.
# plt.figure()
# plt.spy(A, markersize=1, aspect='auto')
# plt.show()

# plt.plot(f0)
# plt.show()

points_p = project(points_3d[point_indices], view_params[view_indices], cam_intrinsics[cam_indices])

plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
plt.plot(points_p[:,0], points_p[:,1], 'ro', markersize=2)
plt.xlim([-1640, 1640])
plt.ylim([-1232, 1232])
plt.show()

#------------------------------------------------------------------------------------------------------------------------
# Optimize.
#------------------------------------------------------------------------------------------------------------------------
t0 = time.time()
# max_nfev=30
res = least_squares(fun, x0, jac_sparsity=A, verbose=2, x_scale='jac', ftol=1e-6, method='trf', args=(n_views, n_points, cam_indices, view_indices, point_indices, points_2d))
t1 = time.time()

#------------------------------------------------------------------------------------------------------------------------
# Print results.
#------------------------------------------------------------------------------------------------------------------------
print("Optimization took {0:.0f} seconds".format(t1 - t0))

# Calculate reprojection error.
f0 = fun(x0, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)
f1 = fun(res.x, n_views, n_points, cam_indices, view_indices, point_indices, points_2d)

view_params_optimized = res.x[:n_views * 6].reshape((n_views, 6))
points_3d_optimized = res.x[n_views * 6:n_views * 6 + n_points * 3].reshape((n_points, 3))
cam_intrinsics_optimized = res.x[n_views * 6 + n_points * 3:n_views * 6 + n_points * 3 + 6].reshape((2, 3))

points_p = project(points_3d_optimized[point_indices], view_params_optimized[view_indices], cam_intrinsics_optimized[cam_indices])

print("Initial RMSE: {}".format(np.sqrt(np.mean(f0**2))))
print("Final RMSE: {}".format(np.sqrt(np.mean(f1**2))))
print("Initial intrinsics: {}".format(cam_intrinsics))
print("Final intrinsics: {}".format(cam_intrinsics_optimized))

# Plot residuals.
plt.plot(f0, 'bo', markersize=2)
plt.plot(f1, 'ro', markersize=2)
plt.show()

# Plot reprojection with optimized params.
plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
plt.plot(points_p[:,0], points_p[:,1], 'ro', markersize=2)
plt.show()

# Plot 3D points after optimization.
# fig = plt.figure()
# ax = fig.add_subplot(111, projection='3d')
# ax.scatter(points_3d[:, 0], points_3d[:, 1], points_3d[:, 2], c='b', marker='o')
# ax.scatter(points_3d_optimized[:, 0], points_3d_optimized[:, 1], points_3d_optimized[:, 2], c='r', marker='o')
# ax.set_aspect('equal')
# plt.show()

# Plot optimized points and cameras.
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.scatter(view_params[:, 3], view_params[:, 4], view_params[:, 5], c='b', marker='o')
ax.scatter(view_params_optimized[:, 3], view_params_optimized[:, 4], view_params_optimized[:, 5], c='r', marker='o')
ax.set_aspect('equal')
plt.show()





