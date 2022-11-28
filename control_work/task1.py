import numpy as np
import json
from matplotlib import pyplot as plt
from matplotlib import collections as mc

task_data = json.load(open("21.json"))
points = task_data["curve"]

points_x = []
points_y = []

for p in points:
    points_x.append(p[0])
    points_y.append(p[1])

points_n = len(points_x)
matrix_n = 2 * points_n - 2

matrix = np.zeros((matrix_n, matrix_n))
vector_x = np.zeros(matrix_n)
vector_y = np.zeros(matrix_n)

for i in range(points_n):
    if i == 0:
        matrix[i][i:i+2] = [2, -1]
        vector_x[i] = points_x[i]
        vector_y[i] = points_y[i]
    elif i == (points_n - 1):
        matrix[2*i-1][2*i-2:2*i] = [-1, 2]
        vector_x[2*i-1] = points_x[i]
        vector_y[2*i-1] = points_y[i]
    else:
        matrix[2*i-1][2*i-1:2*i+1] = [1, 1]
        matrix[2*i][2*i-2:2*i+2] = [1, -2, 2, -1]
        vector_x[2*i-1] = 2*points_x[i]
        vector_y[2*i-1] = 2*points_y[i]
        

control_points_x = np.linalg.solve(matrix, vector_x)
control_points_y = np.linalg.solve(matrix, vector_y)

def lerp(a, b, t):
    return a*(1-t)+b*t

def bezier(p0, p1, p2, p3, t):
    return lerp(lerp(lerp(p0, p1, t), lerp(p1, p2, t), t), lerp(lerp(p1, p2, t), lerp(p2, p3, t), t), t)

bezier_x = []
bezier_y = []

for i in range(points_n - 1):
    for t in np.linspace(0, 1, 100, False):
        bezier_x.append(bezier(points_x[i], control_points_x[2*i], control_points_x[2*i+1], points_x[i+1], t))
        bezier_y.append(bezier(points_y[i], control_points_y[2*i], control_points_y[2*i+1], points_y[i+1], t))

minx = min(np.append(bezier_x, control_points_x))-10
miny = min(np.append(bezier_y, control_points_y))-10
maxx = max(np.append(bezier_x, control_points_x))+10
maxy = max(np.append(bezier_y, control_points_y))+10

fig = plt.figure(1)
ax = plt.axes(xlim=(minx, maxx), ylim=(miny, maxy))
bezier_p = ax.plot(bezier_x, bezier_y, color='darkorchid', linewidth=3)

lines = np.empty(0)
for i in range(points_n - 1):
    lines = np.append(lines, [
            points_x[i],             points_y[i],             control_points_x[2*i],   control_points_y[2*i],
            control_points_x[2*i],   control_points_y[2*i],   control_points_x[2*i+1], control_points_y[2*i+1],
            control_points_x[2*i+1], control_points_y[2*i+1], points_x[i+1],           points_y[i+1]
        ])
lines = np.reshape(lines, (lines.size // 4, 2, 2))

print(lines)

lc = mc.LineCollection(lines, colors='green', linewidths=1.2)
ax.add_collection(lc)

ax.scatter(control_points_x, control_points_y, c='green')
ax.scatter(points_x, points_y, c='darkorchid')

plt.show()