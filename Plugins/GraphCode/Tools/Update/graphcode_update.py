"""GraphCode's updater helper (Docs/GC-UPDATES-DESIGN.md §1.3-1.5).

GraphCode runs this with the ENGINE's own Python (Engine/Binaries/ThirdParty/Python3/Win64), which lives
outside every folder an update moves and reads tar.xz on any Windows. Standard library only, one file:
a swap moves the folder this script came from, and Python has already read it by then.

  stage --cache DIR --dest DIR
      DIR/release.json is the manifest GraphCode already checked the signature of. Check every part and the
      whole archive against it, unpack into DEST, check the tree against its own BUNDLE-SHA256.txt.
  swap --project DIR --staged DIR --pid N --from V --to V [--installed A,B] [--relaunch-exe EXE --relaunch-args ARGS]
      Wait for the editor (pid N) to exit, move every GraphCode folder in Plugins/ to the previous copy and the
      staged ones in, as one unit. With a relaunch: start the editor again and wait until the new GraphCode
      says it started; if the editor ends first, put the previous copy back and start it instead.

Every answer is one JSON line on stdout. The swap also logs to Saved/GraphCode/Update/swap.log.
"""
import argparse
import ctypes
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import time

CHUNK = 1 << 20
BUNDLE_FILES = ("BUNDLE-MANIFEST.json", "BUNDLE-SHA256.txt", "START-HERE.md")


class Refused(Exception):
    """A stop with a reason a person can read; the editor shows it as is."""


def answer(**fields):
    sys.stdout.write(json.dumps(fields) + "\n")
    sys.stdout.flush()


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(CHUNK), b""):
            digest.update(block)
    return digest.hexdigest()


class Parts:
    """The parts, read one after another as the single archive they were cut from."""

    def __init__(self, paths):
        self.paths = list(paths)
        self.current = None

    def read(self, size=-1):
        out = bytearray()
        while size < 0 or len(out) < size:
            if self.current is None:
                if not self.paths:
                    break
                self.current = open(self.paths.pop(0), "rb")
            block = self.current.read(CHUNK if size < 0 else size - len(out))
            if not block:
                self.current.close()
                self.current = None
                continue
            out += block
        return bytes(out)

    def close(self):
        if self.current is not None:
            self.current.close()


def long_path(path):
    """Windows' extended-length form: the bundle's deepest files pass MAX_PATH under a deep project."""
    path = os.path.abspath(path)
    if os.name != "nt" or path.startswith("\\\\?\\"):
        return path
    return "\\\\?\\UNC\\" + path[2:] if path.startswith("\\\\") else "\\\\?\\" + path


def safe_relative(name):
    """A member name as a relative path inside the tree, or None: what tarfile's data filter refuses."""
    parts = name.replace("\\", "/").split("/")
    if name.startswith(("/", "\\")) or any(part in ("..",) or ":" in part for part in parts):
        return None
    parts = [part for part in parts if part not in ("", ".")]
    return os.path.join(*parts) if parts else None


def unpack(paths, root):
    """Stream the archive into root: regular files and folders only, every path checked first."""
    stream = Parts(paths)
    count = 0
    try:
        with tarfile.open(fileobj=stream, mode="r|xz") as archive:
            for member in archive:
                relative = safe_relative(member.name)
                if relative is None or not (member.isfile() or member.isdir()):
                    raise Refused("the archive holds an entry GraphCode never ships: %s" % member.name)
                target = os.path.join(root, relative)
                if member.isdir():
                    os.makedirs(target, exist_ok=True)
                    continue
                os.makedirs(os.path.dirname(target), exist_ok=True)
                source = archive.extractfile(member)
                with open(target, "wb") as out:
                    shutil.copyfileobj(source, out, CHUNK)
                count += 1
    except (tarfile.TarError, EOFError, OSError) as error:
        raise Refused("the archive does not unpack: %s" % error)
    finally:
        stream.close()
    return count


def mutable(relative, shared):
    """The audit's carve-out (Test-GraphCodeRelease $mutableDirectories): live authored folders are listed but not hashed."""
    return any(relative.lower().startswith(directory.rstrip("/").lower() + "/") for directory in shared)


def check_tree(root, manifest):
    bundle = json.load(open(os.path.join(root, "BUNDLE-MANIFEST.json"), encoding="utf-8-sig"))
    if str(bundle.get("version")) != manifest["version"] or str(bundle.get("engineBuildId")) != manifest["engineBuildId"]:
        raise Refused("the archive holds GraphCode %s for engine build %s, not the release it was signed as"
                      % (bundle.get("version"), bundle.get("engineBuildId")))
    shared = [str(item) for item in bundle.get("sharedDirectories", [])]
    listed = set()
    with open(os.path.join(root, "BUNDLE-SHA256.txt"), encoding="utf-8-sig") as lines:
        for line in lines:
            line = line.rstrip("\r\n")
            if not line:
                continue
            digest, sep, relative = line.partition("  ")
            if not sep or len(digest) != 64:
                raise Refused("BUNDLE-SHA256.txt has a line that is not a checksum")
            relative = relative.replace("\\", "/")
            target = os.path.normpath(os.path.join(root, relative))
            if not target.startswith(os.path.normpath(root) + os.sep):
                raise Refused("BUNDLE-SHA256.txt names a path outside the bundle: " + relative)
            listed.add(relative.lower())
            if mutable(relative, shared):
                continue  # present-but-unverified, exactly as the release audit treats it
            if not os.path.isfile(target):
                raise Refused("the archive is missing " + relative)
            if sha256_file(target).lower() != digest.lower():
                raise Refused("the archive's %s does not match its checksum" % relative)
    for folder, _, files in os.walk(root):
        for name in files:
            relative = os.path.relpath(os.path.join(folder, name), root).replace("\\", "/")
            if relative == "BUNDLE-SHA256.txt" or mutable(relative, shared):
                continue
            if relative.lower() not in listed:
                raise Refused("the archive holds a file its checksums do not list: " + relative)
    descriptor = os.path.join(root, "Plugins", "GraphCode", "GraphCode.uplugin")
    if json.load(open(descriptor, encoding="utf-8-sig")).get("VersionName") != manifest["version"]:
        raise Refused("GraphCode.uplugin in the archive does not say " + manifest["version"])


def stage(args):
    manifest = json.load(open(os.path.join(args.cache, "release.json"), encoding="utf-8"))
    whole = hashlib.sha256()
    paths = []
    for part in manifest["parts"]:
        path = os.path.join(args.cache, part["name"])
        if os.path.basename(part["name"]) != part["name"] or not os.path.isfile(path):
            raise Refused("part %s is missing" % part["name"])
        if os.path.getsize(path) != part["size"]:
            raise Refused("part %s is the wrong size" % part["name"])
        digest = hashlib.sha256()
        with open(path, "rb") as stream:
            for block in iter(lambda: stream.read(CHUNK), b""):
                digest.update(block)
                whole.update(block)
        if digest.hexdigest() != part["sha256"]:
            raise Refused("part %s does not match its signed checksum" % part["name"])
        paths.append(path)
    if whole.hexdigest() != manifest["sha256"]:
        raise Refused("the parts do not rebuild the signed archive")

    dest = long_path(args.dest)
    partial = dest + ".partial"
    shutil.rmtree(partial, ignore_errors=True)
    os.makedirs(partial)
    files = unpack(paths, partial)
    if files != manifest.get("files", files):
        raise Refused("the archive holds %d files; its manifest says %s" % (files, manifest.get("files")))
    try:
        check_tree(partial, manifest)
    except (OSError, ValueError, KeyError) as error:
        raise Refused("the unpacked tree cannot be checked: %s" % error)
    shutil.rmtree(dest, ignore_errors=True)
    os.replace(partial, dest)
    answer(ok=True, staged=args.dest, version=manifest["version"], files=files)


# ---- swap -------------------------------------------------------------------------------------------

def wait_for_exit(pid):
    if os.name != "nt":
        return
    kernel32 = ctypes.windll.kernel32
    handle = kernel32.OpenProcess(0x00100000, False, pid)  # SYNCHRONIZE
    if handle:
        kernel32.WaitForSingleObject(handle, 0xFFFFFFFF)
        kernel32.CloseHandle(handle)


def move(source, target, log):
    """Same-drive folder move. A file another program briefly holds (antivirus, indexer) gets a minute."""
    for attempt in range(60):
        try:
            os.replace(source, target)
            return
        except OSError as error:
            if attempt == 59:
                raise
            log("move %s -> %s waits: %s" % (source, target, error))
            time.sleep(1)


def plugin_names(tree):
    folder = os.path.join(tree, "Plugins")
    return sorted(name for name in os.listdir(folder) if os.path.isdir(os.path.join(folder, name))) if os.path.isdir(folder) else []


def relaunch(args):
    """The command line Unreal itself would have restarted with (FUnrealEdMisc::SpawnEditorInstance)."""
    flags = 0x00000008 | 0x00000200 if os.name == "nt" else 0  # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
    return subprocess.Popen('"%s" %s' % (args.relaunch_exe, args.relaunch_args), creationflags=flags, close_fds=True)


def swap(args):
    project = os.path.abspath(args.project)
    work = os.path.join(project, "Saved", "GraphCode", "Update")
    os.makedirs(work, exist_ok=True)
    log_file = open(os.path.join(work, "swap.log"), "a", encoding="utf-8")

    def log(text):
        log_file.write("%s %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S"), text))
        log_file.flush()

    plugins = os.path.join(project, "Plugins")
    previous = os.path.join(work, "previous")
    staged = os.path.abspath(args.staged)
    log("swap %s -> %s from %s, waiting for editor %d" % (args.from_version, args.to_version, staged, args.pid))
    wait_for_exit(args.pid)

    stamp = time.strftime("%Y%m%d-%H%M%S")
    if os.path.normcase(staged) == os.path.normcase(previous):
        # Go back: the previous copy is the source, so it leaves its place before the current one takes it.
        source = os.path.join(work, "going-back-" + stamp)
        move(previous, source, log)
        staged = source
    elif os.path.isdir(previous):
        # One version deep: the older copy goes aside. A bridge an AI client still runs from it may hold an
        # exe, so its removal is best effort and never fails the update (design §1.5).
        aside = os.path.join(work, "previous-old-" + stamp)
        move(previous, aside, log)
        shutil.rmtree(long_path(aside), ignore_errors=True)

    incoming = plugin_names(staged)
    if "GraphCode" not in incoming:
        raise Refused("the staged copy has no Plugins/GraphCode")
    installed = [name for name in (args.installed.split(",") if args.installed else []) if name]
    outgoing = sorted(set(installed) | set(incoming))
    os.makedirs(os.path.join(previous, "Plugins"), exist_ok=True)
    done = []
    try:
        for name in outgoing:
            current = os.path.join(plugins, name)
            if os.path.isdir(current):
                move(current, os.path.join(previous, "Plugins", name), log)
                done.append(("out", name))
        for name in incoming:
            move(os.path.join(staged, "Plugins", name), os.path.join(plugins, name), log)
            done.append(("in", name))
    except OSError as error:
        log("swap failed, undoing: %s" % error)
        for direction, name in reversed(done):
            if direction == "in":
                move(os.path.join(plugins, name), os.path.join(staged, "Plugins", name), log)
            else:
                move(os.path.join(previous, "Plugins", name), os.path.join(plugins, name), log)
        raise Refused("the GraphCode folders could not be swapped (%s); nothing changed" % error)
    json.dump({"version": args.from_version}, open(os.path.join(previous, "version.json"), "w"))
    shutil.rmtree(long_path(staged), ignore_errors=True)
    marker = os.path.join(work, "started-%s.json" % args.to_version)
    if os.path.exists(marker):
        os.remove(marker)
    json.dump({"from": args.from_version, "to": args.to_version, "at": stamp},
              open(os.path.join(work, "update-pending.json"), "w"))
    log("swapped: %s" % ", ".join(incoming))
    if not args.relaunch_exe:
        answer(ok=True, swapped=incoming, relaunched=False)
        return

    editor = relaunch(args)
    log("relaunched editor %d; waiting for GraphCode %s to start" % (editor.pid, args.to_version))
    while True:
        if os.path.exists(marker):
            log("GraphCode %s started" % args.to_version)
            answer(ok=True, swapped=incoming, relaunched=True, started=True)
            return
        if editor.poll() is not None:
            break
        time.sleep(2)

    # The editor ended before the new GraphCode said it started: put the previous copy back.
    if os.path.exists(marker):
        answer(ok=True, swapped=incoming, relaunched=True, started=True)
        return
    log("editor exited (%s) before GraphCode %s started; restoring %s" % (editor.returncode, args.to_version, args.from_version))
    failed = os.path.join(work, "failed-%s-%s" % (args.to_version, stamp), "Plugins")
    os.makedirs(failed, exist_ok=True)
    for name in incoming:
        if os.path.isdir(os.path.join(plugins, name)):
            move(os.path.join(plugins, name), os.path.join(failed, name), log)
    for name in outgoing:
        if os.path.isdir(os.path.join(previous, "Plugins", name)):
            move(os.path.join(previous, "Plugins", name), os.path.join(plugins, name), log)
    shutil.rmtree(long_path(previous), ignore_errors=True)
    os.remove(os.path.join(work, "update-pending.json"))
    json.dump({"version": args.to_version, "restored": args.from_version},
              open(os.path.join(work, "update-failed.json"), "w"))
    relaunch(args)
    log("restored %s and relaunched" % args.from_version)
    answer(ok=False, error="GraphCode %s failed to start, so %s was restored" % (args.to_version, args.from_version),
           rolled_back=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    staging = commands.add_parser("stage")
    staging.add_argument("--cache", required=True)
    staging.add_argument("--dest", required=True)
    swapping = commands.add_parser("swap")
    swapping.add_argument("--project", required=True)
    swapping.add_argument("--staged", required=True)
    swapping.add_argument("--pid", required=True, type=int)
    swapping.add_argument("--from", dest="from_version", required=True)
    swapping.add_argument("--to", dest="to_version", required=True)
    swapping.add_argument("--installed", default="")
    swapping.add_argument("--relaunch-exe", default="")
    swapping.add_argument("--relaunch-args", default="")
    args = parser.parse_args()
    try:
        (stage if args.command == "stage" else swap)(args)
    except Refused as why:
        answer(ok=False, error=str(why))
        sys.exit(2)
    except Exception as error:  # never a silent death: the editor and swap.log both get a line
        answer(ok=False, error="%s: %s" % (type(error).__name__, error))
        sys.exit(1)


if __name__ == "__main__":
    main()
