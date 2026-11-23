"""Headless entry: blender source.blend --background --python this.py -- --help."""
import argparse
from pathlib import Path
import sys

import bpy

sys.path.insert(0,str(Path(__file__).parent/"blender"))
from graphcode_integrate.blender_export import export_material

parser=argparse.ArgumentParser(description="GraphCode Integrate: export a Blender material")
parser.add_argument("--material",help="Exact Blender material name; otherwise use the active object's material")
parser.add_argument("--output",required=True,type=Path)
parser.add_argument("--destination",required=True,help="Unreal package path, e.g. /Game/Integrate/Copper/M_Copper")
parser.add_argument("--mode",choices=("translate","hybrid"),default="hybrid")
parser.add_argument("--resolution",type=int,choices=(256,512,1024,2048,4096),default=2048)
parser.add_argument('--bake-object',help='Exact mesh name for geometry-dependent UV0 bakes; reuse this mesh in Unreal')
parser.add_argument('--surface-policy',choices=('strict','approximate'),default='strict')
args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
material=bpy.data.materials.get(args.material) if args.material else bpy.context.object.active_material if bpy.context.object else None
if material is None and not args.material and len(bpy.data.materials)==1:
    material=bpy.data.materials[0]
if material is None:
    raise ValueError("Choose --material from: "+", ".join(m.name for m in bpy.data.materials))
bake_object=bpy.data.objects.get(args.bake_object) if args.bake_object else None
if args.bake_object and bake_object is None:
    raise ValueError('Bake object not found: '+args.bake_object)
bundle=export_material(material,args.output,args.destination,args.mode,args.resolution,bake_object,args.surface_policy)
print("GraphCode Integrate exported",bundle["asset"],"to",args.output.resolve()/"integrate.json")
