# gc_render's camera as a pinhole: location, look_at, horizontal fov (degrees), image width and height; Unreal axes (X forward,
# Y right, Z up), centimetres. Measured 2026-10-18: a 640-wide view at fov 38 has a 929 px focal length, and from a camera on the
# -X side looking +X, +Y is image right. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
import numpy as np


class Camera:
    def __init__(self, location, look_at, fov, width, height):
        self.location = np.asarray(location, float)
        self.width, self.height = width, height
        f = np.asarray(look_at, float) - self.location
        self.forward = f / np.linalg.norm(f)
        r = np.cross([0.0, 0.0, 1.0], self.forward)
        self.right = r / np.linalg.norm(r)
        self.up = np.cross(self.forward, self.right)
        self.focal = (width / 2.0) / np.tan(np.radians(fov) / 2.0)

    @classmethod
    def from_render(cls, camera, width, height):
        return cls(camera['location'], camera['look_at'], camera['fov'], width, height)

    def project(self, points):
        """Image pixels (x right, y down) of world points (N x 3)."""
        d = np.atleast_2d(points) - self.location
        x, y, z = d @ self.right, d @ self.up, d @ self.forward
        return np.stack([self.width / 2.0 + self.focal * x / z, self.height / 2.0 - self.focal * y / z], 1)

    def matrix(self):
        """The 3 x 4 projection matrix (pixels), for triangulation."""
        K = np.array([[self.focal, 0, self.width / 2.0], [0, -self.focal, self.height / 2.0], [0, 0, 1.0]])
        R = np.stack([self.right, self.up, self.forward])
        return K @ np.hstack([R, -(R @ self.location)[:, None]])
