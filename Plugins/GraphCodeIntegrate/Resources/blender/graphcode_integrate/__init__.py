"""Install this directory as a Blender add-on, or use build-addon.py to make its ZIP."""
bl_info = {
    "name":"GraphCode Integrate", "author":"GraphCode Sdn Bhd", "version":(0,3,0),
    "blender":(4,3,0), "location":"3D View / Shader Editor > Sidebar > GraphCode",
    "description":"Send Blender meshes and editable materials to Unreal through GraphCode",
    "category":"Material",
}

import json
import os
from pathlib import Path
import shutil
import subprocess
import uuid

import bpy
from bpy.props import EnumProperty, StringProperty

from .blender_export import export_material
from .compiler import identifier
from .mesh_export import export_meshes

_job = None
_status = ""
_report = None


class GCIntegratePreferences(bpy.types.AddonPreferences):
    bl_idname = __package__
    project: StringProperty(name="Unreal project folder", subtype="DIR_PATH")
    node: StringProperty(name="Node.js executable", subtype="FILE_PATH", default=shutil.which("node") or "")

    def draw(self, context):
        self.layout.prop(self,"project")
        self.layout.prop(self,"node")


class GCIntegrateSettings(bpy.types.PropertyGroup):
    destination: StringProperty(name="Destination", default="/Game/Integrate/M_Material")
    mesh_destination: StringProperty(name="Destination folder", default="/Game/Integrate/Meshes")
    mode: EnumProperty(name="Conversion", items=[("hybrid","Translate + bake fallback","Keep supported nodes editable; bake unsupported UV branches"),
        ("translate","Translate only","Report any branch that cannot be translated")], default="hybrid")
    resolution: EnumProperty(name="Bake size", items=[(str(v),str(v),"") for v in (256,512,1024,2048,4096)], default="2048")
    surface_policy: EnumProperty(name='Surface mapping',items=[('strict','Strict','Reject surface lobes without a direct mapping'),
        ('approximate','Engine approximation','Allow reported subsurface and thin-glass approximations')],default='strict')
    mesh_bake: bpy.props.BoolProperty(name='Bake on active mesh',description='Freeze geometry-dependent branches using this mesh and UV0; the same mesh is required in Unreal',default=False)


def material_from_context(context):
    if context.space_data and context.space_data.type == "NODE_EDITOR" and context.space_data.shader_type == "OBJECT":
        material = context.space_data.id
        if isinstance(material,bpy.types.Material):
            return material
    return context.object.active_material if context.object else None


def preferences(context):
    return context.preferences.addons[__package__].preferences


def start_import(prefs, project, folder):
    global _job, _status
    resources = project/"Plugins"/"GraphCodeIntegrate"/"Resources"
    native = next((p for p in (resources/"Runtime"/"graphcode-integrate.exe",
                               resources/"graphcode-integrate.exe") if p.is_file()), None)
    if native:
        command = [str(native)]
    else:
        script = resources/"import-bundle.mjs"
        if not script.is_file():
            raise ValueError("Install the GraphCodeIntegrate plugin in the selected Unreal project")
        node = bpy.path.abspath(prefs.node)
        if not Path(node).is_file():
            raise ValueError("Set Node.js for this development checkout, or install the packaged plugin")
        command = [node,str(script)]
    log = folder/"import-result.txt"
    stream = log.open("w",encoding="utf-8")
    try:
        process = subprocess.Popen(command+["--project",str(project),"--bundle",str(folder/"integrate.json"),"--apply","--render"],
            stdout=stream,stderr=subprocess.STDOUT,cwd=str(project),
            creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
    except Exception:
        stream.close()
        raise
    _job = process,stream,log
    _status = "Importing into Unreal…"
    bpy.app.timers.register(poll_job,first_interval=0.5)


def poll_job():
    global _job, _status
    if _job is None:
        return None
    process, stream, log = _job
    if process.poll() is None:
        return 0.5
    stream.close()
    _status = "Imported and verified in Unreal" if process.returncode == 0 else "Import failed; open report for details"
    if process.returncode:
        global _report
        _report = log
    _job = None
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            area.tag_redraw()
    return None


class GCINTEGRATE_OT_export(bpy.types.Operator):
    bl_idname = "graphcode_integrate.export"
    bl_label = "Export material bundle"
    bl_description = "Translate the current material; preserve the original Blender scene"
    send: bpy.props.BoolProperty(default=False, options={"HIDDEN"})

    @classmethod
    def poll(cls, context):
        return _job is None and material_from_context(context) is not None

    def execute(self, context):
        global _job, _status, _report
        try:
            prefs, settings = preferences(context), context.scene.gc_integrate
            project = Path(bpy.path.abspath(prefs.project)).resolve()
            if not prefs.project or not list(project.glob("*.uproject")):
                raise ValueError("Set the Unreal project folder in GraphCode Integrate add-on preferences")
            material = material_from_context(context)
            folder = project/"Saved"/"GraphCodeIntegrate"/"exports"/(identifier(material.name)+"_"+uuid.uuid4().hex[:12])
            _report = folder/"report.json"
            bake_object=context.object if settings.mesh_bake else None
            if settings.mesh_bake and bake_object is None:
                raise ValueError('Select the mesh to bake on')
            export_material(material,folder,settings.destination,settings.mode,int(settings.resolution),bake_object,settings.surface_policy)
            report = json.loads(_report.read_text(encoding="utf-8"))
            _status = f"{len(report['translated'])} outputs translated; {len(report['baked'])} baked; see fidelity report"
            if self.send:
                start_import(prefs, project, folder)
            self.report({"INFO"},_status)
            return {"FINISHED"}
        except Exception as error:
            _status = str(error)
            self.report({"ERROR"},_status)
            return {"CANCELLED"}


class GCINTEGRATE_OT_report(bpy.types.Operator):
    bl_idname = "graphcode_integrate.report"
    bl_label = "Open conversion report"

    def execute(self, context):
        if _report and _report.is_file():
            bpy.ops.wm.path_open(filepath=str(_report))
        return {"FINISHED"}


class GCINTEGRATE_OT_meshes(bpy.types.Operator):
    bl_idname = "graphcode_integrate.meshes"
    bl_label = "Export selected meshes"
    bl_description = "Export evaluated static meshes and converted materials; preserve the source scene"
    send: bpy.props.BoolProperty(default=False, options={"HIDDEN"})

    @classmethod
    def poll(cls, context):
        return _job is None and context.mode == "OBJECT" and any(obj.type == "MESH" for obj in context.selected_objects)

    def execute(self, context):
        global _status, _report
        try:
            prefs, settings = preferences(context), context.scene.gc_integrate
            project = Path(bpy.path.abspath(prefs.project)).resolve()
            if not prefs.project or not list(project.glob("*.uproject")):
                raise ValueError("Set the Unreal project folder in GraphCode Integrate add-on preferences")
            folder = project/"Saved"/"GraphCodeIntegrate"/"exports"/("Meshes_"+uuid.uuid4().hex[:12])
            _report = folder/"report.json"
            bundle = export_meshes([obj for obj in context.selected_objects if obj.type == "MESH"],
                folder, settings.mesh_destination, settings.mode, int(settings.resolution), settings.surface_policy)
            _status = f"Exported {len(bundle['meshes'])} meshes and {len(bundle['materials'])} materials"
            if self.send:
                start_import(prefs, project, folder)
            self.report({"INFO"}, _status)
            return {"FINISHED"}
        except Exception as error:
            _status = str(error)
            self.report({"ERROR"}, _status)
            return {"CANCELLED"}


class GCINTEGRATE_PT_meshes(bpy.types.Panel):
    bl_label = "GraphCode Integrate"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "GraphCode"

    def draw(self, context):
        layout, settings = self.layout, context.scene.gc_integrate
        layout.label(text="Selected static meshes + materials")
        layout.prop(settings, "mesh_destination")
        layout.prop(settings, "mode")
        layout.prop(settings, "surface_policy")
        if settings.mode == "hybrid":
            layout.prop(settings, "resolution")
        layout.operator("graphcode_integrate.meshes", text="Send selected meshes to Unreal").send = True
        layout.operator("graphcode_integrate.meshes", text="Export mesh bundle").send = False
        if _status:
            layout.label(text=_status)
        if _report:
            layout.operator("graphcode_integrate.report")


class GCINTEGRATE_PT_material(bpy.types.Panel):
    bl_label = "GraphCode Integrate"
    bl_space_type = "NODE_EDITOR"
    bl_region_type = "UI"
    bl_category = "GraphCode"

    @classmethod
    def poll(cls, context):
        return context.space_data.tree_type == "ShaderNodeTree"

    def draw(self, context):
        layout, settings = self.layout, context.scene.gc_integrate
        material = material_from_context(context)
        if not material:
            layout.label(text="Select an object with a material")
            return
        layout.label(text=material.name, icon="MATERIAL")
        layout.prop(settings,"destination")
        layout.prop(settings,"mode")
        layout.prop(settings,'surface_policy')
        if settings.mode == "hybrid":
            layout.prop(settings,"resolution")
            layout.prop(settings,'mesh_bake')
        row = layout.row()
        row.enabled = _job is None
        row.operator("graphcode_integrate.export",text="Send to Unreal").send = True
        layout.operator("graphcode_integrate.export",text="Export bundle").send = False
        if _status:
            layout.label(text=_status)
        if _report:
            layout.operator("graphcode_integrate.report")


CLASSES = (GCIntegratePreferences,GCIntegrateSettings,GCINTEGRATE_OT_export,GCINTEGRATE_OT_report,
           GCINTEGRATE_OT_meshes,GCINTEGRATE_PT_material,GCINTEGRATE_PT_meshes)


def register():
    for cls in CLASSES:
        bpy.utils.register_class(cls)
    bpy.types.Scene.gc_integrate = bpy.props.PointerProperty(type=GCIntegrateSettings)


def unregister():
    if hasattr(bpy.types.Scene,"gc_integrate"):
        del bpy.types.Scene.gc_integrate
    for cls in reversed(CLASSES):
        bpy.utils.unregister_class(cls)
