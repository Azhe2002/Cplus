import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.gridspec import GridSpec

print("Reading test.ply ...")
# Parse PLY file
points = []
colors = []
with open('/home/zzhe/Cplus/test.ply', 'r') as f:
    lines = f.readlines()

# Find header end
header_end = 0
for i, line in enumerate(lines):
    if line.strip() == 'end_header':
        header_end = i
        break

# Parse vertex data
for line in lines[header_end + 1:]:
    parts = line.strip().split()
    if len(parts) >= 6:
        points.append([float(parts[0]), float(parts[1]), float(parts[2])])
        colors.append([int(parts[3])/255.0, int(parts[4])/255.0, int(parts[5])/255.0])

points = np.array(points)
colors = np.array(colors)
print(f"Loaded {len(points):,} points")

# Downsample for performance (every Nth point)
step = max(1, len(points) // 150000)
points_ds = points[::step]
colors_ds = colors[::step]
print(f"Downsampled to {len(points_ds):,} points for plotting (step={step})")

# Create figure with two views
fig = plt.figure(figsize=(19.6, 10.8), dpi=100)
fig.patch.set_facecolor('#0a0a0f')

# --- View 1: Perspective from front-right (like the original camera but elevated) ---
ax1 = fig.add_subplot(1, 2, 1, projection='3d')
ax1.set_facecolor('#0a0a0f')
ax1.scatter(points_ds[:, 0], points_ds[:, 1], points_ds[:, 2],
            c=colors_ds, s=1.5, alpha=0.8, marker='.')
ax1.view_init(elev=25, azim=-35)
ax1.set_xlabel('X', color='white')
ax1.set_ylabel('Y', color='white')
ax1.set_zlabel('Z', color='white')
ax1.set_title('Front-right view', color='white', fontsize=12)
ax1.tick_params(colors='white')
ax1.xaxis.pane.fill = False
ax1.yaxis.pane.fill = False
ax1.zaxis.pane.fill = False
ax1.xaxis.pane.set_edgecolor('#222233')
ax1.yaxis.pane.set_edgecolor('#222233')
ax1.zaxis.pane.set_edgecolor('#222233')
ax1.grid(True, alpha=0.2, color='white')

# --- View 2: Top-down view ---
ax2 = fig.add_subplot(1, 2, 2, projection='3d')
ax2.set_facecolor('#0a0a0f')
ax2.scatter(points_ds[:, 0], points_ds[:, 1], points_ds[:, 2],
            c=colors_ds, s=1.5, alpha=0.8, marker='.')
ax2.view_init(elev=85, azim=0)
ax2.set_xlabel('X', color='white')
ax2.set_ylabel('Y', color='white')
ax2.set_zlabel('Z', color='white')
ax2.set_title('Top-down view', color='white', fontsize=12)
ax2.tick_params(colors='white')
ax2.xaxis.pane.fill = False
ax2.yaxis.pane.fill = False
ax2.zaxis.pane.fill = False
ax2.xaxis.pane.set_edgecolor('#222233')
ax2.yaxis.pane.set_edgecolor('#222233')
ax2.zaxis.pane.set_edgecolor('#222233')
ax2.grid(True, alpha=0.2, color='white')

plt.suptitle('Point Cloud Visualization — 1,072,643 points\n(Red/Blue/Green/Yellow spheres + Checkerboard plane)',
             color='white', fontsize=14, y=0.98)
plt.tight_layout()
plt.savefig('/home/zzhe/Cplus/pointcloud.png', dpi=100, facecolor='#0a0a0f',
            bbox_inches='tight', pad_inches=0.3)
print("Saved: pointcloud.png (1960x1080)")
