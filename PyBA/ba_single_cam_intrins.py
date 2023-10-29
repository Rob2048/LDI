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
		n_cameras, n_points, n_observations = map(int, file.readline().split())

		camera_indices = np.empty(n_observations, dtype=int)
		point_indices = np.empty(n_observations, dtype=int)
		points_2d = np.empty((n_observations, 2))

		for i in range(n_observations):
			camera_index, point_index, x, y = file.readline().split()
			camera_indices[i] = int(camera_index)
			point_indices[i] = int(point_index)
			points_2d[i] = [float(x), float(y)]

		camera_params = np.empty(n_cameras * 6)
		for c in range(n_cameras):
			for i in range(6):
				camera_params[c * 6 + i] = float(file.readline())
			
			#Consume the last 3 parameters (f, k1, k2)
			for i in range(3):
				file.readline()
		
		camera_params = camera_params.reshape((n_cameras, -1))

		points_3d = np.empty(n_points * 3)
		for i in range(n_points * 3):
			points_3d[i] = float(file.readline())
		points_3d = points_3d.reshape((n_points, -1))

	return camera_params, points_3d, camera_indices, point_indices, points_2d

def rotate(points, rot_vecs):
	"""Rotate points by given rotation vectors.
	
	Rodrigues' rotation formula is used.
	"""
	theta = np.linalg.norm(rot_vecs, axis=1)[:, np.newaxis]
	with np.errstate(invalid='ignore'):
		v = rot_vecs / theta
		v = np.nan_to_num(v)
	dot = np.sum(points * v, axis=1)[:, np.newaxis]
	cos_theta = np.cos(theta)
	sin_theta = np.sin(theta)

	return cos_theta * points + sin_theta * np.cross(v, points) + dot * (1 - cos_theta) * v

def project(points, camera_params, cam_intrins):
	"""Convert 3-D points to 2-D by projecting onto images."""
	points_proj = rotate(points, camera_params[:, :3])
	points_proj += camera_params[:, 3:6]
	points_proj = -points_proj[:, :2] / points_proj[:, 2, np.newaxis]
	f = cam_intrins[0]
	k1 = cam_intrins[1]
	k2 = cam_intrins[2]
	n = np.sum(points_proj**2, axis=1)
	r = 1 + k1 * n + k2 * n**2
	points_proj *= (r * f)[:, np.newaxis]
	return points_proj

def fun(params, n_cameras, n_points, camera_indices, point_indices, points_2d):
	"""Compute residuals.
	
	`params` contains camera parameters and 3-D coordinates.
	"""
	camera_params = params[:n_cameras * 6].reshape((n_cameras, 6))
	points_3d = params[n_cameras * 6: + n_cameras * 6 + n_points * 3].reshape((n_points, 3))
	cam_intrins = params[n_cameras * 6 + n_points * 3:n_cameras * 6 + n_points * 3 + 3]
	points_proj = project(points_3d[point_indices], camera_params[camera_indices], cam_intrins)
	return (points_proj - points_2d).ravel()

def bundle_adjustment_sparsity(n_cameras, n_points, camera_indices, point_indices):
	m = camera_indices.size * 2
	n = n_cameras * 6 + n_points * 3 + 3
	A = lil_matrix((m, n), dtype=int)

	i = np.arange(camera_indices.size)

	# Poses.
	for s in range(6):
		A[2 * i, camera_indices * 6 + s] = 1
		A[2 * i + 1, camera_indices * 6 + s] = 1

	# Cube points.
	for s in range(3):
		A[2 * i, n_cameras * 6 + point_indices * 3 + s] = 1
		A[2 * i + 1, n_cameras * 6 + point_indices * 3 + s] = 1

	# Camera intrinsics.
	for s in range(3):
		A[2 * i, n_cameras * 6 + n_points * 3 + s] = 1
		A[2 * i + 1, n_cameras * 6 + n_points * 3 + s] = 1

	return A

#------------------------------------------------------------------------------------------------------------------------
# Main.
#------------------------------------------------------------------------------------------------------------------------
camera_params, points_3d, camera_indices, point_indices, points_2d = read_bal_data()

n_cameras = camera_params.shape[0]
n_points = points_3d.shape[0]

cam_intrinsics = np.array([2600, 0, 0])

n = 6 * n_cameras + 3 * n_points + 3
m = 2 * points_2d.shape[0]

print("(all images) n_cameras: {}".format(n_cameras))
print("(3D points) n_points: {}".format(n_points))
print("Total number of parameters: {}".format(n))
print("Total number of residuals: {}".format(m))
print("Total number of camera indices: {}".format(camera_indices.shape[0]))

x0 = np.hstack((camera_params.ravel(), points_3d.ravel(), cam_intrinsics.ravel()))
f0 = fun(x0, n_cameras, n_points, camera_indices, point_indices, points_2d)
A = bundle_adjustment_sparsity(n_cameras, n_points, camera_indices, point_indices)

# Print Jacobian sparsity pattern.
plt.figure()
plt.spy(A, markersize=1, aspect='auto')
plt.show()

plt.plot(f0)
plt.show()

points_p = project(points_3d[point_indices], camera_params[camera_indices], cam_intrinsics)

plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
plt.plot(points_p[:,0], points_p[:,1], 'ro', markersize=2)
plt.show()

t0 = time.time()
# max_nfev=30
res = least_squares(fun, x0, jac_sparsity=A, verbose=2, x_scale='jac', ftol=1e-8, method='trf',
 					args=(n_cameras, n_points, camera_indices, point_indices, points_2d))
t1 = time.time()

print("Optimization took {0:.0f} seconds".format(t1 - t0))

plt.plot(res.fun)
plt.show()

camera_params_optimized = res.x[:n_cameras * 6].reshape((n_cameras, 6))
points_3d_optimized = res.x[n_cameras * 6:n_cameras * 6 + n_points * 3].reshape((n_points, 3))
cam_intrinsics_optimized = res.x[n_cameras * 6 + n_points * 3:n_cameras * 6 + n_points * 3 + 3]

points_p = project(points_3d_optimized[point_indices], camera_params_optimized[camera_indices], cam_intrinsics_optimized)

plt.plot(points_2d[:,0], points_2d[:,1], 'bo', markersize=2)
plt.plot(points_p[:,0], points_p[:,1], 'ro', markersize=2)

plt.show()

# Calculate reprojection error.
f0 = fun(x0, n_cameras, n_points, camera_indices, point_indices, points_2d)
f1 = fun(res.x, n_cameras, n_points, camera_indices, point_indices, points_2d)

plt.plot(f0, 'bo', markersize=2)
plt.plot(f1, 'ro', markersize=2)
plt.show()

print("Initial RMSE: {}".format(np.sqrt(np.mean(f0**2))))
print("Final RMSE: {}".format(np.sqrt(np.mean(f1**2))))

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
ax.scatter(points_3d_optimized[:, 0], points_3d_optimized[:, 1], points_3d_optimized[:, 2], c='r', marker='o')
ax.scatter(camera_params_optimized[:, 3], camera_params_optimized[:, 4], camera_params_optimized[:, 5], c='b', marker='o')
ax.set_aspect('equal')
plt.show()

# Plot change in camera intrinsics
# plt.plot(camera_params[:, 7], 'bo', markersize=2)
# plt.plot(camera_params_optimized[:, 7], 'ro', markersize=2)
# plt.show()

# Print intrinsics
print("Initial intrinsics: {}".format(cam_intrinsics))
print("Final intrinsics: {}".format(cam_intrinsics_optimized))



