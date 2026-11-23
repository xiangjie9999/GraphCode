# A rendered take made to look as a generated video of it would: one colour grade for the whole take (exposure, white balance, gamma,
# saturation, a hue turn), a new background behind the character, a little softness and grain, then H.264 at a modest quality and
# back to frames through the same reader a real video takes (track_all.frames_from_video). The motion and the camera stay exactly the
# render's, so every joint is still known: what changes is only how the pictures look. Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md (E16).
# Usage: python ai_wash.py <take_dir> <out_dir> [seed] [crf]
import glob, json, os, shutil, subprocess, sys
import numpy as np
import cv2
from PIL import Image, ImageFilter
import track_all


def silhouette(a):
    """As dino_reader.silhouette: all but the near-black that reaches the picture's edge (lit far legs stand in shadow)."""
    m = cv2.morphologyEx(((a.max(2) > 3) * 255).astype(np.uint8), cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    outside = (m == 0).astype(np.uint8); h, w = outside.shape; fill = np.zeros((h + 2, w + 2), np.uint8)
    for x, y in [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1), (w // 2, 0), (w // 2, h - 1), (0, h // 2), (w - 1, h // 2)]:
        if outside[y, x] == 1: cv2.floodFill(outside, fill, (x, y), 2)
    return outside != 2


def grade(rng):
    gain, lift, gamma, wb, sat, hue = rng.uniform(0.8, 1.3), rng.uniform(-15, 20), rng.uniform(0.8, 1.25), rng.uniform(0.85, 1.15, 3), \
        rng.uniform(0.6, 1.5), rng.uniform(-25, 25)
    def apply(a):
        a = np.clip((a.astype(np.float32) * gain + lift) * wb, 0, 255)
        a = 255.0 * (a / 255.0) ** gamma
        hsv = cv2.cvtColor(a.astype(np.uint8), cv2.COLOR_RGB2HSV).astype(np.float32)
        hsv[..., 0] = (hsv[..., 0] + hue / 2) % 180; hsv[..., 1] = np.clip(hsv[..., 1] * sat, 0, 255)
        return cv2.cvtColor(hsv.astype(np.uint8), cv2.COLOR_HSV2RGB)
    return apply


def backdrop(rng, h, w):
    """A soft room-like backdrop: a floor-to-wall gradient with low-frequency blotches."""
    top, bottom = rng.integers(40, 220, 3), rng.integers(20, 200, 3)
    t = np.linspace(0, 1, h)[:, None, None]
    base = top * (1 - t) + bottom * t
    blot = np.asarray(Image.fromarray(rng.integers(0, 255, (h // 60 + 2, w // 60 + 2, 3)).astype(np.uint8)).resize((w, h), Image.BICUBIC), np.float32)
    return np.clip(base * 0.8 + blot * 0.2, 0, 255)


def main():
    take, out = sys.argv[1], sys.argv[2]
    rng = np.random.default_rng(int(sys.argv[3]) if len(sys.argv) > 3 else 16)
    crf = sys.argv[4] if len(sys.argv) > 4 else '26'
    os.makedirs(out, exist_ok=True)
    for name in ('cameras.json', 'first.json', 'frames.json'):
        if os.path.exists(os.path.join(take, name)): shutil.copy(os.path.join(take, name), out)
    cameras = json.load(open(os.path.join(take, 'cameras.json')))
    work = os.path.join(out, '_wash'); os.makedirs(work, exist_ok=True)
    for c in cameras:
        files = sorted(glob.glob(os.path.join(take, '%s_[0-9][0-9][0-9].png' % c)))
        if not files: continue
        look = grade(rng)
        h, w = np.asarray(Image.open(files[0])).shape[:2]
        back = backdrop(rng, h, w)
        for k, f in enumerate(files):
            a = np.asarray(Image.open(f).convert('RGB'))
            m = silhouette(a)
            m = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8)), np.float32)[..., None] / 255.0
            a = look(a).astype(np.float32) * m + back * (1 - m)
            a = a + rng.standard_normal(a.shape).astype(np.float32) * 3.0
            Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.6)).save(os.path.join(work, '%s_%03d.png' % (c, k)))
        video = os.path.join(out, c + '.mp4')
        subprocess.check_call(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '30', '-i', os.path.join(work, c + '_%03d.png'),
                               '-c:v', 'libx264', '-crf', crf, '-pix_fmt', 'yuv420p', video])
        print('%s: %d frames back from %s' % (c, track_all.frames_from_video(video, c, out, (w, h)), video))
    shutil.rmtree(work)


if __name__ == '__main__':
    main()
