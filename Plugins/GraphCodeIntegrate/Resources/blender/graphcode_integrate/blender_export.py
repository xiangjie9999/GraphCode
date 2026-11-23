"""Read the active shader, flatten group instances, stage images and bake UV branches.

Source materials, node groups and scenes are never edited. Bakes use a disposable scene.
"""
import hashlib
import json
from array import array
from pathlib import Path

import bpy

from .compiler import Compiler, TranslationError, identifier


def save_exr(image, filename, scene=None):
    owned_scene = scene is None
    scene = scene or bpy.data.scenes.new('GCIntegrate_Save')
    try:
        # Image.colorspace_settings can reset Image.file_format to PNG in Blender 5.2.
        # An explicit render output format writes real linear EXR, including negative/HDR data.
        scene.render.image_settings.file_format = 'OPEN_EXR'
        scene.render.image_settings.color_depth = '32'
        scene.render.image_settings.color_mode = 'RGBA'
        image.save_render(str(filename),scene=scene)
    finally:
        if owned_scene:
            bpy.data.scenes.remove(scene)


def plain(value):
    if isinstance(value,dict):
        return {key:plain(item) for key,item in value.items()}
    if isinstance(value, (str, int, float, bool)):
        return value
    try:
        return list(value)
    except TypeError:
        return None


class Snapshot:
    def __init__(self, material, directory):
        self.material = material
        self.directory = Path(directory)
        self.nodes = {}
        self.originals = {}
        self.images = {}

    def socket(self, socket, frames):
        if not socket.is_linked:
            return None
        link = socket.links[0]
        return self.output(link.from_node, list(link.from_node.outputs).index(link.from_socket), frames)

    def output(self, node, index, frames):
        key = "/".join([f[0].name for f in frames] + [node.name])
        animation = node.id_data.animation_data
        if animation and (animation.action or animation.drivers):
            raise TranslationError(f"{key}: animated shader graphs need an explicit frame/animation export workflow")
        if node.type == "REROUTE":
            return self.socket(node.inputs[0], frames)
        if node.mute:
            output = node.outputs[index]
            internal = next((l for l in node.internal_links if l.to_socket == output), None)
            if internal:
                return self.socket(internal.from_socket, frames)
            raise TranslationError(f"{key}: muted output has no input connection")
        if node.type == "GROUP":
            output = next((n for n in node.node_tree.nodes if n.type == "GROUP_OUTPUT" and n.is_active_output), None)
            if output is None:
                raise TranslationError(f"{key}: group has no active output")
            return self.socket(output.inputs[index], frames + [(node, frames)])
        if node.type == "GROUP_INPUT":
            if not frames:
                raise TranslationError("Group input outside a group instance")
            group, outer = frames[-1]
            source = group.inputs[index]
            if source.is_linked:
                return self.socket(source, outer)
            key = "/".join([f[0].name for f in frames]) + "/input:" + source.identifier
            self.nodes[key] = {"type":"Parameter", "name":source.name, "inputs":[],
                "props":{"name":group.label or group.name + "_" + source.name,
                         "value":plain(source.default_value), "kind":source.type}}
            return [key, 0]
        if key in self.nodes:
            return [key, index]
        record = {"type":node.bl_idname, "name":node.name, "label":node.label, "inputs":[], "props":{}}
        self.nodes[key] = record
        self.originals[key] = node
        for socket in node.inputs:
            item = {"name":socket.name, "identifier":socket.identifier, "kind":socket.type}
            if hasattr(socket, "default_value"):
                item["value"] = plain(socket.default_value)
            connection = self.socket(socket, frames)
            if connection:
                item["link"] = connection
            record["inputs"].append(item)
        for name in ("operation", "use_clamp", "blend_type", "data_type", "factor_mode", "clamp_factor",
                     "clamp_result", "vector_type", "mode", "projection", "extension", "interpolation",
                     "uv_map", "from_instancer", "space", "invert", "inside", "only_local", "samples"):
            if hasattr(node, name):
                record["props"][name] = plain(getattr(node, name))
        if node.type in ("VALUE", "RGB"):
            record["props"]["value"] = plain(node.outputs[0].default_value)
        if node.type == "VALTORGB":
            ramp = node.color_ramp
            record["props"].update(color_mode=ramp.color_mode, interpolation=ramp.interpolation,
                elements=[[e.position, list(e.color)] for e in ramp.elements])
        if node.type == "TEX_IMAGE" and node.image:
            record["props"]["texture"] = self.stage_image(node.image)
        return [key, index]

    def stage_image(self, image):
        if image.name in self.images:
            return self.images[image.name]
        if image.source not in ("FILE", "GENERATED") or image.size[0] == 0:
            raise TranslationError(f"{image.name}: tiled, sequence, movie or missing images need a dedicated import path")
        folder = self.directory / "textures"
        folder.mkdir(parents=True, exist_ok=True)
        key = identifier(image.name) + "_" + hashlib.sha256(image.name.encode()).hexdigest()[:8]
        is_hdr = image.is_float
        path = folder / (key + (".exr" if is_hdr else ".png"))
        copied = bpy.data.images.new("GCIntegrate_Stage",width=image.size[0],height=image.size[1],
                                     alpha=True,float_buffer=is_hdr)
        try:
            # Image.copy() copies the ID, not its loaded/painted pixel buffer.
            pixels = array('f',[0]) * len(image.pixels)
            image.pixels.foreach_get(pixels)
            copied.alpha_mode = image.alpha_mode
            copied.filepath_raw = str(path)
            copied.file_format = "OPEN_EXR" if is_hdr else "PNG"
            # Preserve the source interpretation; HDR output format is set on a disposable scene.
            copied.colorspace_settings.name = "Non-Color" if is_hdr else image.colorspace_settings.name
            copied.pixels.foreach_set(pixels)
            if is_hdr:
                save_exr(copied,path)
            else:
                copied.save()
        finally:
            bpy.data.images.remove(copied)
        source_linear = image.colorspace_settings.is_data or image.colorspace_settings.name in ("Linear Rec.709","Linear")
        result = {"key":key, "file":str(path), "linear":is_hdr or source_linear,
                  "source_colorspace":image.colorspace_settings.name,
                  "sha256":hashlib.sha256(path.read_bytes()).hexdigest()}
        # Data and standard sRGB have defined Unreal counterparts. Do not relabel ACEScg as sRGB.
        if not source_linear and image.colorspace_settings.name != "sRGB":
            raise TranslationError(f"{image.name}: convert {image.colorspace_settings.name} to sRGB before exporting")
        self.images[image.name] = result
        return result

    def read(self):
        if not self.material.use_nodes:
            raise TranslationError("Material must use shader nodes")
        output = next((n for n in self.material.node_tree.nodes if n.type == "OUTPUT_MATERIAL" and n.is_active_output), None)
        if output is None or not output.inputs["Surface"].is_linked:
            raise TranslationError("Material has no connected active Surface output")
        surface = self.socket(output.inputs["Surface"], [])
        displacement = self.socket(output.inputs["Displacement"], [])
        if output.inputs["Volume"].is_linked:
            raise TranslationError("Volume materials require a volume shading translation")
        return {"nodes":self.nodes, "surface":surface, "displacement":displacement,
                "displacement_method":self.material.displacement_method,
                "blender_version":bpy.app.version_string, "material":self.material.name}


class UVBaker:
    ALLOWED = {"VALUE", "RGB", "MATH", "VECT_MATH", "MAPPING", "MIX", "MIX_RGB", "VALTORGB",
        "TEX_NOISE", "TEX_VORONOI", "TEX_WAVE", "TEX_CHECKER", "TEX_BRICK", "TEX_IMAGE", "TEX_COORD", "UVMAP",
        "SEPXYZ", "COMBXYZ", "SEPRGB", "COMBRGB", "SEPARATE_COLOR", "COMBINE_COLOR", "RGBTOBW", "INVERT", "HUE_SAT", "GAMMA", "BRIGHTCONTRAST", "CLAMP", "MAP_RANGE"}

    def __init__(self, snapshot, resolution=2048, bake_object=None):
        if resolution not in (256, 512, 1024, 2048, 4096):
            raise TranslationError("Bake resolution must be 256, 512, 1024, 2048 or 4096")
        self.snapshot = snapshot
        self.resolution = resolution
        self.bake_object = bake_object
        if bake_object is not None and (bake_object.type!='MESH' or not bake_object.data.uv_layers):
            raise TranslationError('Mesh baking needs a mesh with an unwrapped UV0 channel')

    def dependencies(self, key, output, visited=None):
        visited = set() if visited is None else visited
        pair = (key, output)
        if pair in visited:
            return visited
        visited.add(pair)
        record = self.snapshot.nodes[key]
        original = self.snapshot.originals.get(key)
        if original:
            if original.type not in self.ALLOWED and not (self.bake_object and original.type=='AMBIENT_OCCLUSION'):
                raise TranslationError(f"{key}: {original.bl_idname} cannot be baked as an object-independent UV branch")
            if original.type == "TEX_COORD" and output != 2 and not (self.bake_object and output in (0,3)):
                raise TranslationError(f"{key}: this branch depends on geometry/space; a flat UV bake would change its meaning")
            if original.type == "UVMAP" and (original.from_instancer or (original.uv_map and (not self.bake_object or original.uv_map!=self.bake_object.data.uv_layers[0].name))):
                raise TranslationError(f"{key}: named/instanced UVs need a mesh and channel mapping")
            if original.type.startswith("TEX_") and original.type not in ("TEX_COORD", "TEX_IMAGE"):
                if not record["inputs"][0].get("link") and not self.bake_object:
                    raise TranslationError(f"{key}: connect UV coordinates before baking; implicit Generated coordinates are object-dependent")
            if original.id_data.animation_data and (original.id_data.animation_data.action or original.id_data.animation_data.drivers):
                raise TranslationError(f"{key}: animated shader graphs need an explicit frame/animation export workflow")
        for socket in record["inputs"]:
            if "link" in socket:
                self.dependencies(*socket["link"], visited)
        return visited

    def __call__(self, key, output, reason):
        dependencies = self.dependencies(key, output)
        scene = bpy.data.scenes.new("GCIntegrate_Bake")
        material = bpy.data.materials.new("GCIntegrate_Bake")
        mesh = (bpy.data.meshes.new_from_object(self.bake_object.evaluated_get(bpy.context.evaluated_depsgraph_get()))
                if self.bake_object else bpy.data.meshes.new("GCIntegrate_Bake"))
        obj = bpy.data.objects.new("GCIntegrate_Bake", mesh)
        image = None
        occluders=[]
        try:
            scene.collection.objects.link(obj)
            if self.bake_object:
                obj.matrix_world=self.bake_object.matrix_world.copy()
                mesh.uv_layers.active_index=0
                mesh.uv_layers[0].active_render=True
                if any(not -1e-6<=c<=1.000001 for item in mesh.uv_layers[0].data for c in item.uv):
                    raise TranslationError('Mesh baking needs UV0 inside [0,1]; unwrap without overlapping islands before export')
                # AO sees the evaluated source scene, including nearby occluding meshes.
                if any(self.snapshot.nodes[k]['type']=='ShaderNodeAmbientOcclusion' for k,_ in dependencies):
                    for source in bpy.context.scene.objects:
                        if source==self.bake_object or source.type!='MESH' or source.hide_render:
                            continue
                        data=bpy.data.meshes.new_from_object(source.evaluated_get(bpy.context.evaluated_depsgraph_get()))
                        other=bpy.data.objects.new('GCIntegrate_Occluder',data)
                        other.matrix_world=source.matrix_world.copy()
                        scene.collection.objects.link(other)
                        occluders.append((other,data))
                mesh.materials.clear()
                for polygon in mesh.polygons:
                    polygon.material_index=0
            else:
                mesh.from_pydata([(-.5,-.5,0),(.5,-.5,0),(.5,.5,0),(-.5,.5,0)], [], [(0,1,2,3)])
                layer = mesh.uv_layers.new()
                for loop, uv in zip(layer.data, [(0,0),(1,0),(1,1),(0,1)]):
                    loop.uv = uv
            mesh.materials.append(material)
            material.use_nodes = True
            tree = material.node_tree
            tree.nodes.clear()
            clones = {}

            def clone(node_key):
                if node_key in clones:
                    return clones[node_key]
                record = self.snapshot.nodes[node_key]
                original = self.snapshot.originals.get(node_key)
                if original is None:
                    value = record["props"]["value"]
                    node = tree.nodes.new("ShaderNodeRGB" if isinstance(value,list) else "ShaderNodeValue")
                    node.outputs[0].default_value = value if not isinstance(value,list) else value[:3]+[value[3] if len(value)>3 else 1]
                else:
                    node = tree.nodes.new(original.bl_idname)
                    for prop in original.bl_rna.properties:
                        name = prop.identifier
                        if prop.is_readonly or prop.type not in ("BOOLEAN", "INT", "FLOAT", "STRING", "ENUM"):
                            continue
                        if name in ("name", "location", "location_absolute", "dimensions", "width", "height", "select", "parent"):
                            continue
                        try:
                            setattr(node, name, getattr(original, name))
                        except (TypeError, AttributeError):
                            pass
                    if original.type == "TEX_IMAGE":
                        node.image = original.image
                    if original.type=='TEX_COORD':
                        node.object=original.object
                    if original.type == "VALTORGB":
                        src, dst = original.color_ramp, node.color_ramp
                        dst.color_mode, dst.interpolation, dst.hue_interpolation = src.color_mode, src.interpolation, src.hue_interpolation
                        while len(dst.elements)>2:
                            dst.elements.remove(dst.elements[-1])
                        for i, element in enumerate(src.elements):
                            target = dst.elements[i] if i<2 else dst.elements.new(element.position)
                            target.position, target.color = element.position, element.color
                    for i, socket in enumerate(original.outputs):
                        if hasattr(socket,"default_value"):
                            node.outputs[i].default_value = socket.default_value
                clones[node_key] = node
                for i, socket in enumerate(record["inputs"]):
                    if "link" in socket:
                        source, slot = socket["link"]
                        tree.links.new(clone(source).outputs[slot],node.inputs[i])
                    elif "value" in socket and hasattr(node.inputs[i],"default_value"):
                        node.inputs[i].default_value = socket["value"]
                return node

            value = clone(key).outputs[output]
            emission = tree.nodes.new("ShaderNodeEmission")
            surface = tree.nodes.new("ShaderNodeOutputMaterial")
            tree.links.new(value,emission.inputs["Color"])
            tree.links.new(emission.outputs[0],surface.inputs["Surface"])
            image = bpy.data.images.new("GCIntegrate_Bake", width=self.resolution, height=self.resolution, float_buffer=True, alpha=False)
            image.colorspace_settings.name = "Non-Color"
            target = tree.nodes.new("ShaderNodeTexImage")
            target.image = image
            tree.nodes.active = target
            obj.select_set(True, view_layer=scene.view_layers[0])
            scene.view_layers[0].objects.active = obj
            scene.render.engine = "CYCLES"
            scene.cycles.samples = 64 if any(self.snapshot.nodes[k]['type']=='ShaderNodeAmbientOcclusion' for k,_ in dependencies) else 1
            scene.render.bake.margin = 8 if self.bake_object else 0
            scene.render.bake.use_clear = True
            with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0], active_object=obj,
                    object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
                bpy.ops.object.bake(type="EMIT")
            # EXR preserves negative/HDR branch values; PNG would silently clamp them.
            digest = hashlib.sha256((key+":"+str(output)).encode()).hexdigest()[:12]
            filename = self.snapshot.directory / "textures" / ("Bake_" + digest + ".exr")
            filename.parent.mkdir(parents=True, exist_ok=True)
            save_exr(image,filename,scene)
            frozen = [name for name,_ in sorted(dependencies) if self.snapshot.nodes[name]["type"] in ("Parameter","ShaderNodeValue","ShaderNodeRGB")]
            return {"texture":{"key":"Bake_"+digest, "file":str(filename), "linear":True,
                        "sha256":hashlib.sha256(filename.read_bytes()).hexdigest()},
                    "kind":"VALUE" if value.type == "VALUE" else "RGBA",
                    "report":{"source":key,"socket":output,"reason":reason,"resolution":self.resolution,
                        "frozen_controls":sorted(set(frozen)), "domain":"mesh UV0 [0,1]" if self.bake_object else "UV0 [0,1]", "tiling":"not guaranteed",
                        "mesh":self.mesh_identity() if self.bake_object else None,
                        "file":str(filename)}}
        finally:
            for other,data in occluders:
                bpy.data.objects.remove(other,do_unlink=True)
                bpy.data.meshes.remove(data)
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(mesh)
            bpy.data.materials.remove(material)
            if image:
                bpy.data.images.remove(image)
            bpy.data.scenes.remove(scene)

    def mesh_identity(self):
        obj=self.bake_object
        payload={'vertices':[list(v.co) for v in obj.data.vertices],
                 'polygons':[list(p.vertices) for p in obj.data.polygons],
                 'uv0':[list(v.uv) for v in obj.data.uv_layers[0].data]}
        return {'object':obj.name,'geometry_uv_sha256':hashlib.sha256(json.dumps(payload).encode()).hexdigest(),
                'matrix_world':[list(row) for row in obj.matrix_world],
                'requires':'Use the same evaluated mesh, UV0 and source scale; scene-dependent AO is frozen. UV islands must not overlap.'}


def export_material(material, directory, destination, mode="hybrid", resolution=2048, bake_object=None, surface_policy='strict'):
    if mode not in ("hybrid", "translate"):
        raise TranslationError("Mode must be translate or hybrid")
    directory = Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    # Never leave the previous successful manifest importable after a failed re-export.
    (directory/"integrate.json").unlink(missing_ok=True)
    snapshot = Snapshot(material, directory)
    try:
        graph = snapshot.read()
        result = Compiler(graph, destination, UVBaker(snapshot,resolution,bake_object) if mode == "hybrid" else None,
                          surface_policy=surface_policy).compile()
    except Exception as error:
        (directory/"report.json").write_text(json.dumps({"status":"failed","material":material.name,
            "blender_version":bpy.app.version_string,"error":str(error)},indent=2),encoding="utf-8")
        raise
    (directory/"material.gc").write_text(result["document"],encoding="utf-8")
    provenance = {"material":material.name, "blend_file":bpy.data.filepath,
        "blender_version":bpy.app.version_string}
    metadata = material.get("asset_data")
    if metadata:
        # Keep attribution, not BlenderKit's signed download URLs or account metadata.
        for name in ("assetBaseId", "id", "name", "license", "author"):
            value = metadata.get(name)
            if value is not None:
                provenance[name] = value.to_dict() if hasattr(value,"to_dict") else plain(value)
    result["report"]["source"] = provenance
    (directory/"report.json").write_text(json.dumps(result["report"],indent=2),encoding="utf-8")
    bundle = {"format":"graphcode-integrate", "version":1, "integration":"blender-material",
        "asset":destination, "document":"material.gc", "textures":result["textures"], "source":provenance,
        "report":"report.json"}
    for texture in bundle["textures"]:
        texture["file"] = str(Path(texture["file"]).relative_to(directory)).replace("\\","/")
    (directory/"integrate.json").write_text(json.dumps(bundle,indent=2),encoding="utf-8")
    return bundle
