# The joint ball of a one-bone render: its pink pixels (the bone's own colour when it is the only bone drawn) against a render without
# the overlay. Returns the ball's centroid in pixels, or None. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md.
import numpy as np


def overlay_mask(plain, drawn, threshold=30):
    return np.abs(np.asarray(drawn, float) - np.asarray(plain, float)).sum(2) > threshold


def ball(plain, drawn):
    B = np.asarray(drawn, float)
    m = overlay_mask(plain, drawn)
    pink = m & (B[..., 0] > 150) & (B[..., 0] - B[..., 1] > 50) & (B[..., 0] - B[..., 2] > 50)
    ys, xs = np.where(pink)
    if len(xs) < 6: return None, len(xs)
    return np.array([xs.mean(), ys.mean()]), len(xs)


def circle_from(p1, p2, p3):
    ax, ay = p1; bx, by = p2; cx, cy = p3
    d = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by))
    if abs(d) < 1e-9: return None
    ux = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) + (cx * cx + cy * cy) * (ay - by)) / d
    uy = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) + (cx * cx + cy * cy) * (bx - ax)) / d
    return np.array([ux, uy]), np.hypot(ax - ux, ay - uy)


def ball_circle(plain, drawn, tries=400, tolerance=0.9, radius=(3.0, 20.0), seed=0):
    """The joint ball's ring fitted as a circle (RANSAC, then least squares on its inliers): the ball's cross and the bone's side tick,
    drawn in the same colour, are outliers rather than weight in a centroid (2026-10-18: the centroid sat 1-2 px off on some bones,
    always the same way). Returns the centre (pixel index coordinates), the radius, and the inlier count; None when no ring is found."""
    B = np.asarray(drawn, float)
    m = overlay_mask(plain, drawn)
    pink = m & (B[..., 0] > 150) & (B[..., 0] - B[..., 1] > 50) & (B[..., 0] - B[..., 2] > 50)
    ys, xs = np.where(pink)
    P = np.stack([xs, ys], 1).astype(float)
    if len(P) < 10: return None, 0, 0
    rng = np.random.default_rng(seed)
    best = None
    for _ in range(tries):
        i = rng.choice(len(P), 3, replace=False)
        c = circle_from(*P[i])
        if c is None or not (radius[0] <= c[1] <= radius[1]): continue
        d = np.abs(np.hypot(*(P - c[0]).T) - c[1])
        n = (d < tolerance).sum()
        if best is None or n > best[0]: best = (n, c)
    if best is None: return None, 0, 0
    inl = P[np.abs(np.hypot(*(P - best[1][0]).T) - best[1][1]) < tolerance]
    # least squares refinement (algebraic, Kasa) on the inliers
    A = np.column_stack([2 * inl[:, 0], 2 * inl[:, 1], np.ones(len(inl))])
    b = (inl ** 2).sum(1)
    cx, cy, k = np.linalg.lstsq(A, b, rcond=None)[0]
    r = np.sqrt(max(k + cx * cx + cy * cy, 0))
    return np.array([cx, cy]), r, len(inl)
