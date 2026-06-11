"""
Draw red bounding boxes around the 4 spheres in pointcloud.png
Uses the exact 3D scene geometry + perspective projection.
"""
import numpy as np
from PIL import Image, ImageDraw, ImageFont

# ---- Same camera as raytracer ----
WIDTH, HEIGHT = 1960, 1080
cam = np.array([0.0, 1.5, -2.0])
fov = np.deg2rad(60)
aspect = WIDTH / HEIGHT
tan_half = np.tan(fov / 2)
sx = 1.0 / (tan_half * aspect)
sy = 1.0 / tan_half

# ---- Sphere definitions (from pointcloud.cpp) ----
spheres = [
    {"name": "Red sphere",    "center": np.array([-1.0,  0.5,  5.0]), "radius": 1.5, "color": (255, 0, 0)},
    {"name": "Blue sphere",   "center": np.array([ 2.0, -0.5,  4.0]), "radius": 1.0, "color": (0, 100, 255)},
    {"name": "Green sphere",  "center": np.array([-2.5,  0.0,  3.5]), "radius": 0.8, "color": (0, 255, 0)},
    {"name": "Yellow sphere", "center": np.array([ 0.5, -1.0,  3.0]), "radius": 0.5, "color": (255, 200, 0)},
]

def project(world_pt):
    """Project a 3D world point to 2D pixel coordinates (same math as project_pc.cpp)"""
    c = world_pt - cam  # camera space
    if c[2] <= 0.001:
        return None
    px = c[0] / c[2]
    py = c[1] / c[2]
    fx = (px * sx + 1.0) * 0.5 * WIDTH - 0.5
    fy = (1.0 - py * sy) * 0.5 * HEIGHT - 0.5
    return np.array([fx, fy])

def project_sphere_aabb(center, radius):
    """Project the 8 corners of a sphere's AABB, return (min_xy, max_xy) in pixels."""
    corners = []
    for dx in [-radius, radius]:
        for dy in [-radius, radius]:
            for dz in [-radius, radius]:
                pt = center + np.array([dx, dy, dz])
                px = project(pt)
                if px is not None:
                    corners.append(px)

    # Also sample points on the sphere surface for a tighter box
    # Sample many points, project them, take the convex hull extent
    n_samples = 200
    for i in range(n_samples):
        # Fibonacci sphere sampling
        phi = np.arccos(1 - 2 * (i + 0.5) / n_samples)
        theta = np.pi * (1 + np.sqrt(5)) * i
        dx = radius * np.sin(phi) * np.cos(theta)
        dy = radius * np.sin(phi) * np.sin(theta)
        dz = radius * np.cos(phi)
        pt = center + np.array([dx, dy, dz])
        px = project(pt)
        if px is not None:
            corners.append(px)

    if not corners:
        return None
    corners = np.array(corners)
    return corners.min(axis=0), corners.max(axis=0)

# ---- Load image and draw ----
img = Image.open('/home/zzhe/Cplus/pointcloud.png')
draw = ImageDraw.Draw(img)

# Try to load a font, fallback to default
try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 22)
    font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
except:
    font = ImageFont.load_default()
    font_small = ImageFont.load_default()

for i, s in enumerate(spheres):
    result = project_sphere_aabb(s["center"], s["radius"])
    if result is None:
        print(f"{s['name']}: cannot project (behind camera?)")
        continue

    pmin, pmax = result
    x1, y1 = int(pmin[0]), int(pmin[1])
    x2, y2 = int(pmax[0]), int(pmax[1])

    # Clamp to image bounds
    x1, y1 = max(0, x1), max(0, y1)
    x2, y2 = min(WIDTH-1, x2), min(HEIGHT-1, y2)

    # Draw bounding box (3px thick red outline)
    for offset in [-1, 0, 1]:
        draw.rectangle([x1-offset, y1-offset, x2+offset, y2+offset],
                       outline=s["color"], width=2)

    # Label above the box
    label = f"{s['name']} r={s['radius']}"
    tw, th = font_small.getbbox(label)[2:] if hasattr(font_small, 'getbbox') else (200, 18)

    # Label background
    ly = max(0, y1 - th - 8)
    draw.rectangle([x1, ly, x1 + tw + 12, y1], fill=(*s["color"], 180))
    draw.text((x1 + 6, ly + 2), label, fill=(255, 255, 255), font=font_small)

    # Project center for a dot
    cp = project(s["center"])
    if cp is not None:
        cx, cy = int(cp[0]), int(cp[1])
        r_dot = 4
        draw.ellipse([cx-r_dot, cy-r_dot, cx+r_dot, cy+r_dot], fill=(255, 255, 255), outline=s["color"], width=2)

    w, h = x2 - x1, y2 - y1
    print(f"{s['name']}: box=({x1},{y1})-({x2},{y2})  size={w}x{h}px")

# Title bar
title = "Point Cloud — 4 Spheres + Checkerboard (re-projected from 3D)"
draw.rectangle([0, 0, WIDTH, 42], fill=(0, 0, 0, 200))
draw.text((20, 10), title, fill=(255, 255, 255), font=font)

img.save('/home/zzhe/Cplus/pointcloud_boxed.png')
print("\nSaved: pointcloud_boxed.png")
