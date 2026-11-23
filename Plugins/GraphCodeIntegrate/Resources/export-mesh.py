"""blender source.blend --background --python-exit-code 1 --python export-mesh.py -- --help"""
import argparse
from pathlib import Path
import sys
import bpy

sys.path.insert(0, str(Path(__file__).parent / "blender"))
from graphcode_integrate.mesh_export import export_meshes

parser = argparse.ArgumentParser(description="Export Blender static meshes and converted materials")
parser.add_argument("--object", action="append", dest="objects", help="Exact object name; repeat for multiple meshes (default: selected meshes)")
parser.add_argument("--output", required=True, type=Path)
parser.add_argument("--destination", required=True, help="Unreal destination folder, e.g. /Game/Integrate/Props")
parser.add_argument("--mode", choices=("translate", "hybrid"), default="hybrid")
parser.add_argument("--resolution", type=int, choices=(256,512,1024,2048,4096), default=2048)
parser.add_argument("--surface-policy", choices=("strict", "approximate"), default="strict")
args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
objects = [bpy.data.objects.get(name) for name in args.objects] if args.objects else [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
if any(obj is None for obj in objects):
    raise ValueError("An --object name was not found")
bundle = export_meshes(objects, args.output, args.destination, args.mode, args.resolution, args.surface_policy)
print("GraphCode Integrate exported", len(bundle["meshes"]), "meshes to", args.output.resolve()/"integrate.json")
