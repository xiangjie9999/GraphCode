"""Memory headroom for editor-side geometry inspection; never closes applications."""
import ctypes
import gc
import sys

GIB = 1024 ** 3
MIN_FREE_RAM = 4 * GIB
MIN_FREE_COMMIT = 6 * GIB
_operations = 0


class MemoryBudgetExceeded(RuntimeError):
    pass


class _MemoryStatus(ctypes.Structure):
    _fields_ = [("length", ctypes.c_uint32), ("load", ctypes.c_uint32)] + [
        (name, ctypes.c_uint64) for name in (
            "total_ram", "free_ram", "total_commit", "free_commit",
            "total_virtual", "free_virtual", "extended_virtual")]


def available():
    if sys.platform != "win32":
        raise MemoryBudgetExceeded("Geometry memory guard requires a supported Windows memory probe.")
    status = _MemoryStatus()
    status.length = ctypes.sizeof(status)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
        raise MemoryBudgetExceeded("Cannot measure memory headroom; inspection stopped.")
    return {"free_ram": status.free_ram, "free_commit": status.free_commit}


def release():
    # Kismet SystemLibrary.collect_garbage only requests GC for a later frame.
    # Python's native binding performs it now, while a long script is running.
    import unreal
    gc.collect()
    unreal.collect_garbage()


def checkpoint(force=False):
    global _operations
    _operations += 1
    state = available()
    low = state["free_ram"] < MIN_FREE_RAM or state["free_commit"] < MIN_FREE_COMMIT
    if force or low or _operations % 24 == 0:
        release()
        state = available()
    if state["free_ram"] < MIN_FREE_RAM or state["free_commit"] < MIN_FREE_COMMIT:
        raise MemoryBudgetExceeded(
            "Inspection stopped to preserve working memory: "
            f"{state['free_ram']/GIB:.1f} GiB RAM and {state['free_commit']/GIB:.1f} GiB commit free; "
            f"requires {MIN_FREE_RAM/GIB:g}/{MIN_FREE_COMMIT/GIB:g} GiB. "
            "Remaining geometry has not been checked.")
    return state
