"""Blender shader graph -> GraphCode. No bpy dependency; no Unreal writes.

The graph is a flattened snapshot, with socket indices (not ambiguous display names)
as connection identities. An optional baker evaluates unsupported, UV-only branches.
"""
import hashlib
import json
import math
import re
from dataclasses import dataclass


class TranslationError(ValueError):
    pass


class Unsupported(TranslationError):
    pass


def identifier(value):
    result = re.sub(r"[^A-Za-z0-9_]", "_", value).strip("_") or "Material"
    return "_" + result if result[0].isdigit() else result


def number(value):
    value = float(value)
    if not math.isfinite(value):
        raise TranslationError("Non-finite shader value")
    return format(value, ".9g")


def quote(value):
    return json.dumps(str(value), ensure_ascii=True)


@dataclass(frozen=True)
class Signal:
    ref: str
    kind: str = "VALUE"


class Compiler:
    def __init__(self, graph, destination, baker=None, surface_policy='strict', opacity='masked'):
        if not re.fullmatch(r"/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+", destination):
            raise TranslationError("Destination must be an Unreal /Game/package path")
        self.graph = graph
        self.destination = destination
        self.baker = baker
        self.surface_policy = surface_policy
        self.opacity = opacity
        self.lines = []
        self.cache = {}
        self.visiting = set()
        self.textures = {}
        self.parameters = {}
        self.report = {"translated": [], "baked": [], "limitations": [], "parameters": []}

    def emit(self, cls, inputs=(), props=None, kind="VALUE"):
        alias = "n" + str(len(self.lines))
        args, fields = [], []
        for index, name, signal in inputs:
            args.append(f"in{index}={signal.ref}")
            fields.append(f"link{index}.name={quote(name)}")
        fields.extend(f"{key}={value}" for key, value in (props or {}).items())
        self.lines.append(f"  {alias} = {cls}({', '.join(args)})" +
                          (f" [{', '.join(fields)}]" if fields else ""))
        return Signal(alias + ".out0", kind)

    def constant(self, value, kind=None):
        if isinstance(value, (tuple, list)):
            if len(value) < 3:
                value = list(value) + [0] * (3 - len(value))
            return self.emit("Constant3Vector", props={"Constant":
                "(R=%s,G=%s,B=%s,A=1)" % tuple(number(v) for v in value[:3])}, kind=kind or "RGBA")
        return self.emit("Constant", props={"R": number(value)}, kind="VALUE")

    def parameter(self, key, name, value, kind):
        # The suffix prevents collisions between equally named inputs in different group instances.
        name = identifier(name) + "_" + hashlib.sha256(key.encode()).hexdigest()[:8]
        if name not in self.parameters:
            if kind in ("RGBA", "VECTOR"):
                fields = {"ParameterName": name, "DefaultValue":
                    "(R=%s,G=%s,B=%s,A=%s)" % tuple(number(x) for x in list(value[:3]) + [value[3] if len(value) > 3 else 1])}
                signal = self.emit("VectorParameter", props=fields, kind=kind)
                signal = Signal(signal.ref + ".rgb", kind)
            else:
                signal = self.emit("ScalarParameter", props={"ParameterName": name, "DefaultValue": number(value)})
            self.parameters[name] = signal
            self.report["parameters"].append({"name": name, "source": key, "default": value})
        return self.parameters[name]

    def binary(self, cls, a, b, kind=None):
        return self.emit(cls, [(0, "A", a), (1, "B", b)], kind=kind or a.kind)

    def unary(self, cls, a, props=None):
        return self.emit(cls, [(0, "Input", a)], props, a.kind)

    def mix(self, a, b, factor):
        return self.emit("LinearInterpolate", [(0, "A", a), (1, "B", b), (2, "Alpha", factor)], kind=a.kind)

    def clamp(self, a):
        return self.unary("Saturate", a)

    def cast(self, a, kind):
        if kind in ("RGBA", "VECTOR") and a.kind == "VALUE":
            return self.binary("Multiply",self.constant((1,1,1),kind),a,kind)
        if kind == "VALUE" and a.kind in ("RGBA", "VECTOR"):
            # Blender uses luminance for colours and an arithmetic mean for vectors.
            weights = (0.2126, 0.7152, 0.0722) if a.kind == "RGBA" else (1/3, 1/3, 1/3)
            return self.emit("DotProduct", [(0, "A", a), (1, "B", self.constant(weights))])
        return Signal(a.ref, kind)

    def input(self, node, name, default=0):
        sockets = node.get("inputs", [])
        socket = sockets[name] if isinstance(name, int) and name < len(sockets) else next(
            (s for s in sockets if s["name"] == name), None)
        if socket is None:
            return self.constant(default)
        if "link" in socket:
            return self.cast(self.output(*socket["link"]), socket["kind"])
        return self.constant(socket.get("value", default), socket.get("kind"))

    def output(self, node_id, socket_index=0):
        key = (node_id, socket_index)
        if key in self.cache:
            return self.cache[key]
        if key in self.visiting:
            raise TranslationError(f"Shader cycle at {node_id}")
        self.visiting.add(key)
        node = self.graph["nodes"][node_id]
        checkpoint = (len(self.lines), dict(self.cache), dict(self.textures), dict(self.parameters),
                      {name:len(self.report[name]) for name in ("translated","baked","parameters")})
        try:
            try:
                result = self.translate(node_id, node, socket_index)
                self.report["translated"].append({"source": node_id, "type": node["type"], "socket": socket_index})
            except Unsupported as error:
                if self.baker is None:
                    raise Unsupported(f"{node_id}: {error}") from error
                length, self.cache, self.textures, self.parameters, counts = checkpoint
                del self.lines[length:]
                for name,count in counts.items():
                    del self.report[name][count:]
                baked = self.baker(node_id, socket_index, str(error))
                self.report["baked"].append(baked["report"])
                result = self.texture(baked["texture"], self.uv(), baked.get("kind", "RGBA"))
            self.cache[key] = result
            return result
        finally:
            self.visiting.remove(key)

    def uv(self):
        uv = self.emit("TextureCoordinate")
        v = self.binary("Subtract",self.constant(1),Signal(uv.ref+".g"))
        xy = self.emit("AppendVector",[(0,"A",Signal(uv.ref+".r")),(1,"B",v)],kind="VECTOR")
        return self.emit("AppendVector", [(0, "A", xy), (1, "B", self.constant(0))], kind="VECTOR")

    def texture(self, texture, uv, kind="RGBA", normal=False):
        key = texture["key"] + ("_normal" if normal else "_data" if texture.get("linear") else "_color")
        asset = self.destination.rsplit("/", 1)[0] + "/T_" + identifier(key)
        self.textures[asset] = {**texture, "asset": asset, "role": "normal" if normal else "data" if texture.get("linear") else "color"}
        props = {"Texture": "obj'" + asset + "." + asset.rsplit("/", 1)[1] + "'",
                 "SamplerType": "SAMPLERTYPE_Normal" if normal else "SAMPLERTYPE_LinearColor" if texture.get("linear") else "SAMPLERTYPE_Color"}
        # Procedural arithmetic runs in Blender UV space; image samplers use Unreal's V direction.
        base = uv.ref.removesuffix(".rgb")
        v = self.binary("Subtract",self.constant(1),Signal(base+".g"))
        coordinates = self.emit("AppendVector",[(0,"A",Signal(base+".r")),(1,"B",v)],kind="VECTOR")
        sample = self.emit("TextureSample", [(0, "Coordinates", coordinates)], props, kind)
        return Signal(sample.ref + (".r" if kind == "VALUE" else ".rgb"), kind)

    def translate(self, key, node, out):
        typ, p = node["type"], node.get("props", {})
        get = lambda name, default=0: self.input(node, name, default)
        if typ == "Parameter":
            return self.parameter(key, p["name"], p["value"], p["kind"])
        if typ in ("ShaderNodeValue", "ShaderNodeRGB"):
            kind = "VALUE" if typ.endswith("Value") else "RGBA"
            return self.parameter(key, node.get("label") or node["name"], p["value"], kind)
        if typ == "ShaderNodeTexCoord":
            if out == 2:
                return self.uv()
            raise Unsupported("Only UV coordinates are portable in this version; generated/object coordinates need an explicit space contract")
        if typ == "ShaderNodeUVMap":
            if p.get("uv_map") or p.get("from_instancer"):
                raise Unsupported("Named or instanced UV maps need an explicit Unreal UV-channel mapping")
            return self.uv()
        if typ == "ShaderNodeTexImage":
            if p.get("projection", "FLAT") != "FLAT" or p.get("extension", "REPEAT") != "REPEAT" or p.get("interpolation", "Linear") != "Linear":
                raise Unsupported("Image projection, extension or interpolation requires baking")
            if "texture" not in p:
                raise TranslationError(f"{key}: image is missing")
            uv = get("Vector") if node["inputs"][0].get("link") else self.uv()
            result = self.texture(p["texture"], uv)
            return Signal(result.ref.removesuffix(".rgb").replace(".out0",".out4"), "VALUE") if out == 1 else result
        if typ == "ShaderNodeMath":
            a = get(0)
            op = p["operation"]
            binary = {"ADD": "Add", "SUBTRACT": "Subtract", "MULTIPLY": "Multiply", "MINIMUM": "Min", "MAXIMUM": "Max"}
            unary = {"ABSOLUTE": "Abs", "FLOOR": "Floor", "CEIL": "Ceil", "FRACT": "Frac", "SQRT": "SquareRoot", "SIGN": "Sign"}
            if op in binary:
                result = self.binary(binary[op], a, get(1))
            elif op in unary:
                if op == "SQRT":
                    a = self.binary("Max", a, self.constant(0))
                result = self.unary(unary[op], a)
            elif op in ("SINE", "COSINE"):
                result = self.unary("Sine" if op == "SINE" else "Cosine", a, {"Period": number(2 * math.pi)})
            elif op == "MULTIPLY_ADD":
                result = self.binary("Add", self.binary("Multiply", a, get(1)), get(2))
            elif op in ("LESS_THAN", "GREATER_THAN"):
                zero, one = self.constant(0), self.constant(1)
                result = self.emit("If", [(0, "A", a), (1, "B", get(1)), (2, "AGreaterThanB", one if op == "GREATER_THAN" else zero),
                    (3, "AEqualsB", zero), (4, "ALessThanB", one if op == "LESS_THAN" else zero)], {"EqualsThreshold": "0"})
            else:
                raise Unsupported(f"Math {op} has no verified equivalent yet")
            return self.clamp(result) if p.get("use_clamp") else result
        if typ == "ShaderNodeVectorMath":
            op = p["operation"]
            if op in ("ADD", "SUBTRACT", "MULTIPLY", "MINIMUM", "MAXIMUM"):
                return self.binary({"ADD":"Add", "SUBTRACT":"Subtract", "MULTIPLY":"Multiply", "MINIMUM":"Min", "MAXIMUM":"Max"}[op], get(0), get(1))
            if op == "SCALE":
                return self.binary("Multiply", get(0), get("Scale", 1))
            if op == "DOT_PRODUCT":
                return self.emit("DotProduct", [(0, "A", get(0)), (1, "B", get(1))])
            if op == "LENGTH":
                return self.emit("Length", [(0, "V", get(0))])
            raise Unsupported(f"Vector Math {op} requires baking")
        if typ=='ShaderNodeInvert':
            colour=get('Color',(1,1,1,1))
            return self.mix(colour,self.binary('Subtract',self.constant((1,1,1)),colour),get('Fac',1))
        if typ in ("ShaderNodeMixRGB", "ShaderNodeMix"):
            if p.get("blend_type", "MIX") != "MIX":
                raise Unsupported("Only the Mix blend mode is currently translated")
            if typ == "ShaderNodeMixRGB":
                factor, a, b = get(0), get(1), get(2)
            else:
                dtype = p.get("data_type", "FLOAT")
                if dtype == "VECTOR" and p.get("factor_mode") == "NON_UNIFORM":
                    factor = get(1)
                else:
                    factor = get(0)
                offset = {"FLOAT":2, "VECTOR":4, "RGBA":6}.get(dtype)
                if offset is None:
                    raise Unsupported(f"Mix {dtype} is unsupported")
                a, b = get(offset), get(offset + 1)
            if typ == "ShaderNodeMixRGB" or p.get("clamp_factor", True):
                factor = self.clamp(factor)
            result = self.mix(a, b, factor)
            return self.clamp(result) if p.get("use_clamp") or p.get("clamp_result") else result
        if typ == "ShaderNodeValToRGB":
            if p["color_mode"] != "RGB" or p["interpolation"] not in ("LINEAR", "CONSTANT", "EASE"):
                raise Unsupported("This colour-ramp interpolation requires baking")
            factor = self.clamp(get(0))
            stops = p["elements"]
            colour = lambda e: self.constant(e[1][3] if out == 1 else e[1][:3])
            result = colour(stops[0])
            for left, right in zip(stops, stops[1:]):
                if p["interpolation"] == "CONSTANT" or right[0] == left[0]:
                    result = self.emit("If", [(0,"A",factor), (1,"B",self.constant(right[0])),
                        (2,"AGreaterThanB",colour(right)), (3,"AEqualsB",colour(right)), (4,"ALessThanB",result)],
                        {"EqualsThreshold":"0"}, result.kind)
                else:
                    t = self.clamp(self.binary("Divide", self.binary("Subtract",factor,self.constant(left[0])),self.constant(right[0]-left[0])))
                    if p["interpolation"] == "EASE":
                        t = self.binary("Multiply", self.binary("Multiply",t,t), self.binary("Subtract",self.constant(3),self.binary("Multiply",self.constant(2),t)))
                    result = self.mix(result, colour(right), t)
            return result
        if typ in ("ShaderNodeSeparateXYZ", "ShaderNodeSeparateRGB", "ShaderNodeSeparateColor"):
            if p.get("mode", "RGB") != "RGB":
                raise Unsupported("HSV/HSL separation requires baking")
            return Signal(get(0).ref.removesuffix(".rgb") + "." + "rgb"[out], "VALUE")
        if typ in ("ShaderNodeCombineXYZ", "ShaderNodeCombineRGB", "ShaderNodeCombineColor"):
            if p.get("mode", "RGB") != "RGB":
                raise Unsupported("HSV/HSL combination requires baking")
            xy = self.emit("AppendVector", [(0,"A",get(0)), (1,"B",get(1))])
            return self.emit("AppendVector", [(0,"A",xy), (1,"B",get(2))], kind="VECTOR" if typ.endswith("XYZ") else "RGBA")
        if typ == "ShaderNodeMapping":
            if p.get("vector_type") != "POINT":
                raise Unsupported("Only Point mapping is translated")
            rotation = node["inputs"][2]
            if "link" in rotation or any(abs(v)>1e-9 for v in rotation["value"]):
                raise Unsupported("Rotated mapping requires baking in this version")
            return self.binary("Add",self.binary("Multiply",get("Vector"),get("Scale", (1,1,1))),get("Location", (0,0,0)))
        if typ == "ShaderNodeTexChecker":
            if "link" not in node["inputs"][0]:
                raise Unsupported("Checker needs explicit UV coordinates")
            pos = self.binary("Multiply",get("Vector"),get("Scale",5))
            # Blender offsets/scales checker coordinates to avoid precision artefacts on cell boundaries.
            pos = self.binary("Multiply", self.binary("Add",pos,self.constant(0.000001)), self.constant(0.999999))
            xyz = [self.unary("Abs",self.unary("Floor",Signal(pos.ref+"."+c))) for c in "rgb"]
            total = self.binary("Add",self.binary("Add",xyz[0],xyz[1]),xyz[2])
            parity = self.binary("Fmod",total,self.constant(2))
            return parity if out == 1 else self.mix(get("Color2"),get("Color1"),parity)
        raise Unsupported(f"{typ} output {out} requires an evaluated bake")

    def normal(self, connection):
        key, _ = connection
        node = self.graph["nodes"][key]
        if node['type'] == 'ShaderNodeBump':
            normal_input = next(s for s in node['inputs'] if s['name']=='Normal')
            base = self.normal(normal_input['link']) if 'link' in normal_input else self.emit('VertexNormalWS',kind='VECTOR')
            distance = self.input(node,'Distance',1)
            if node['props'].get('invert'):
                distance = self.binary('Multiply',distance,self.constant(-1))
            result = self.bump(self.input(node,'Height'),base,distance,self.input(node,'Strength',1))
            self.report['translated'].append({'source':key,'type':node['type'],'socket':0})
            self.report['limitations'].append('Bump uses screen derivatives. Blender Filter Width controls its finite-difference sampling step, not height amplitude; that filter radius is not reproduced.')
            return result
        if node["type"] != "ShaderNodeNormalMap" or node["props"].get("space") != "TANGENT" or node["props"].get("uv_map"):
            raise TranslationError(f"{key}: normal translation currently requires a Tangent Space Normal Map using UV0")
        colour = self.input(node,"Color",(.5,.5,1,1))
        decoded = self.binary("Subtract",self.binary("Multiply",colour,self.constant(2)),self.constant(1))
        strength = self.binary("Max",self.input(node,"Strength",1),self.constant(0))
        xy = self.binary("Multiply",Signal(decoded.ref+".rg","VECTOR"),strength)
        z = self.mix(self.constant(1),Signal(decoded.ref+".b"),self.clamp(strength))
        vector = self.emit("AppendVector",[(0,"A",xy),(1,"B",z)],kind="VECTOR")
        result = self.emit("Normalize",[(0,"VectorInput",vector)],kind="VECTOR")
        self.report["translated"].append({"source":key,"type":node["type"],"socket":0})
        self.report["limitations"].append("Normals require the same UV0 tangent basis as Blender; no normal-channel flip is guessed.")
        return self.emit('Transform',[(0,'Input',result)],
            {'TransformSourceType':'TRANSFORMSOURCE_Tangent','TransformType':'TRANSFORM_World'},'VECTOR')

    def bump(self, height, normal, distance, strength=None):
        """Surface-gradient bump in metres; accepts an already perturbed world normal."""
        normalize = lambda value: self.emit('Normalize',[(0,'VectorInput',value)],kind='VECTOR')
        derivative = lambda cls,value: self.emit(cls,[(0,'Value',value)],kind=value.kind)
        normal = normalize(normal)
        position = self.binary('Multiply',self.emit('WorldPosition',props={
            'WorldPositionShaderOffset':'WPT_ExcludeAllShaderOffsets'},kind='VECTOR'),self.constant(.01))
        dx,dy = derivative('DDX',position),derivative('DDY',position)
        rx = self.binary('CrossProduct',dy,normal,'VECTOR')
        ry = self.binary('CrossProduct',normal,dx,'VECTOR')
        determinant = self.binary('DotProduct',dx,rx,'VALUE')
        gradient = self.binary('Add',self.binary('Multiply',rx,derivative('DDX',height)),
            self.binary('Multiply',ry,derivative('DDY',height)))
        magnitude = self.binary('Multiply',distance,self.emit('TwoSidedSign'))
        offset = self.binary('Multiply',gradient,self.binary('Multiply',magnitude,self.unary('Sign',determinant)))
        # DDX/DDY are derivatives, not differences sampled at Blender's Filter Width.
        # Dividing by that width here would incorrectly amplify default bump by ten.
        normal_weight = self.unary('Abs',determinant)
        # At degenerate screen derivatives preserve the input instead of normalizing zero.
        normal_weight = self.binary('Max',normal_weight,self.constant(1e-20))
        perturbed = normalize(self.binary('Subtract',self.binary('Multiply',normal,normal_weight),offset))
        return normalize(self.mix(normal,perturbed,self.binary('Max',strength or self.constant(1),self.constant(0))))

    def compile(self):
        node_id, _ = self.graph["surface"]
        shader = self.graph["nodes"][node_id]
        glass = None
        glass_factor = None
        if shader['type']=='ShaderNodeMixShader' and self.surface_policy=='approximate':
            sockets=shader['inputs']
            lobes=[self.graph['nodes'].get(s.get('link',[None])[0]) for s in sockets[1:3]]
            if {n['type'] for n in lobes if n} == {'ShaderNodeBsdfGlass','ShaderNodeBsdfPrincipled'}:
                glass_index=next(i for i,n in enumerate(lobes) if n['type']=='ShaderNodeBsdfGlass')
                glass=lobes[glass_index]
                glass_factor=self.clamp(self.input(shader,0))
                if glass_index==0:
                    glass_factor=self.binary('Subtract',self.constant(1),glass_factor)
                shader=lobes[1-glass_index]
                self.report['limitations'].append('Glass/Principled shader mixture uses a single thin-translucent lobe with blended roughness, normal and reflectance; it is an approximation, not two independent BSDFs.')
        if shader["type"] != "ShaderNodeBsdfPrincipled":
            raise TranslationError("The active surface must resolve to one Principled BSDF; shader mixtures cannot be flattened faithfully")
        # Never silently discard linked or non-default lobes.
        requirements = {"Coat Weight":0, "Sheen Weight":0,
                        "Anisotropic":0, "Thin Film Thickness":0,
                        "Specular Tint": [1,1,1,1]}
        if self.surface_policy!='approximate':
            requirements.update({'Subsurface Weight':0,'Transmission Weight':0})
        for socket in shader["inputs"]:
            if socket["name"] in requirements:
                expected, value = requirements[socket["name"]], socket.get("value")
                same = (all(abs(a-b)<1e-6 for a,b in zip(value,expected)) if isinstance(expected,list)
                        else isinstance(value,(int,float)) and abs(value-expected)<1e-6)
                if "link" in socket or not same:
                    raise TranslationError(f"{node_id}: {socket['name']} needs a shading model mapping that this version does not yet provide")
            if socket["name"] in ("Tangent", "Coat Normal") and "link" in socket:
                raise TranslationError(f"{node_id}: linked {socket['name']} requires tangent/bump translation; not silently discarded")
        settings = {}
        active = lambda name,default=0: any(s['name']==name and ('link' in s or s.get('value',default)!=default) for s in shader['inputs'])
        transmission = glass is not None or active('Transmission Weight')
        subsurface = active('Subsurface Weight')
        if not transmission:
            settings['bUsedWithNanite']='true'
        if transmission and subsurface:
            raise TranslationError('Simultaneous subsurface and transmission requires a layered shading model')
        channels = {"BaseColor":self.input(shader,"Base Color",(0.8,0.8,0.8,1)),
                    "Metallic":self.input(shader,"Metallic",0), "Roughness":self.input(shader,"Roughness",0.5)}
        ior = self.binary('Max',self.input(shader,'IOR',1.5),self.constant(1e-5))
        ratio = self.binary('Divide',self.binary('Subtract',ior,self.constant(1)),self.binary('Add',ior,self.constant(1)))
        f0 = self.binary('Multiply',ratio,ratio)
        channels['Specular'] = self.binary('Multiply',self.binary('Multiply',f0,self.constant(25)),self.input(shader,'Specular IOR Level',.5))
        self.report['specular'] = {'mapping':'25 * IOR Level * ((IOR-1)/(IOR+1))^2',
            'unreal_range':[0,1],'note':'Unreal clamps its scalar Specular input; reflectance above 8% needs a broader shading model.'}
        emission = self.binary("Multiply",self.input(shader,"Emission Color",(0,0,0,1)), self.input(shader,"Emission Strength",1))
        channels["EmissiveColor"] = emission
        normal = next((s for s in shader["inputs"] if s["name"]=="Normal" and "link" in s),None)
        if normal:
            channels["Normal"] = self.normal(normal["link"])
            settings['bTangentSpaceNormal'] = 'false'
        if subsurface:
            weight=self.clamp(self.input(shader,'Subsurface Weight'))
            settings['ShadingModel']='MSM_Subsurface'
            channels['SubsurfaceColor']=self.binary('Multiply',channels['BaseColor'],weight)
            channels['Opacity']=self.binary('Subtract',self.constant(1),weight)
            self.report['subsurface']={'mapping':'Unreal legacy Subsurface', 'fidelity':'approximate',
                'source_controls':{s['name']:s.get('link',s.get('value')) for s in shader['inputs'] if s['name'].startswith('Subsurface')}}
            self.report['limitations'].append('Legacy Subsurface preserves weight and colour but cannot reproduce Blender random-walk radius, scale, IOR or anisotropy. Those source controls are recorded, not claimed as translated.')
        if transmission:
            amount=self.clamp(self.input(shader,'Transmission Weight'))
            tint=channels['BaseColor']
            if glass is not None:
                amount=self.mix(amount,self.constant(1),glass_factor)
                tint=self.mix(tint,self.input(glass,'Color',(1,1,1,1)),glass_factor)
                channels['Roughness']=self.mix(channels['Roughness'],self.input(glass,'Roughness'),glass_factor)
                gi=self.input(glass,'IOR',1.5)
                gr=self.binary('Divide',self.binary('Subtract',gi,self.constant(1)),self.binary('Add',gi,self.constant(1)))
                channels['Specular']=self.mix(channels['Specular'],self.binary('Multiply',self.binary('Multiply',gr,gr),self.constant(12.5)),glass_factor)
                gn=next((s for s in glass['inputs'] if s['name']=='Normal' and 'link' in s),None)
                if gn:
                    channels['Normal']=self.emit('Normalize',[(0,'VectorInput',self.mix(channels.get('Normal') or self.emit('VertexNormalWS',kind='VECTOR'),self.normal(gn['link']),glass_factor))],kind='VECTOR')
                    settings['bTangentSpaceNormal']='false'
            amount=self.binary('Multiply',amount,self.binary('Subtract',self.constant(1),channels['Metallic']))
            channels['Opacity']=self.binary('Subtract',self.constant(1),amount)
            self.emit('ThinTranslucentMaterialOutput',[(0,'TransmittanceColor',self.clamp(tint)),(1,'SurfaceCoverage',self.clamp(self.input(shader,'Alpha',1)))])
            settings.update({'BlendMode':'BLEND_Translucent','ShadingModel':'MSM_ThinTranslucent','TranslucencyLightingMode':'TLM_SurfacePerPixelLighting','TwoSided':'true'})
            self.report['transmission']={'mapping':'thin translucent sheet','fidelity':'approximate'}
            self.report['limitations'].append('Transmission uses a thin sheet: coloured transmission and surface reflection are retained; volumetric refraction, thickness and caustics are not reproduced.')
        elif active('Alpha',1):
            if self.opacity!='masked':
                raise TranslationError('Continuous alpha with these surface lobes requires a translucent mapping; select masked only for cutout materials')
            settings.update({'BlendMode':'BLEND_Masked','OpacityMaskClipValue':'0.5','TwoSided':'true'})
            channels['OpacityMask']=self.input(shader,'Alpha',1)
            self.report['opacity']={'mode':'masked','threshold':.5}
            self.report['limitations'].append('Alpha is a cutout mask at 0.5; fractional coverage is thresholded, not continuous transparency.')
        if self.graph.get("displacement"):
            key, _ = self.graph["displacement"]
            displacement = self.graph["nodes"][key]
            if displacement["type"] != "ShaderNodeDisplacement" or displacement["props"].get("space") != "OBJECT":
                raise TranslationError(f"{key}: displacement currently requires the scalar Displacement node in Object space")
            def setting(name):
                socket = next(s for s in displacement["inputs"] if s["name"]==name)
                if "link" in socket:
                    raise TranslationError(f"{key}: animated/linked displacement {name} needs bounded Nanite scaling")
                return float(socket["value"])
            scale, center = setting("Scale"), setting("Midlevel")
            if not 0 <= center <= 1:
                raise TranslationError(f"{key}: Nanite displacement Center requires a midlevel between 0 and 1")
            if any(s["name"]=="Normal" and "link" in s for s in displacement["inputs"]):
                raise TranslationError(f"{key}: directional/vector displacement requires a vector displacement mapping")
            height = self.input(displacement,"Height")
            method = self.graph.get('displacement_method','BUMP')
            if transmission and method!='BUMP':
                self.report['limitations'].append('Nanite tessellation does not support this translucent surface. Displacement is retained as bump; silhouette displacement requires exported geometry.')
                method='BUMP'
            if method in ('BUMP','BOTH'):
                channels['Normal'] = self.bump(height,channels.get('Normal') or self.emit('VertexNormalWS',kind='VECTOR'),self.constant(scale))
                settings['bTangentSpaceNormal'] = 'false'
                self.report['bump'] = {'source':key,'distance_metres':scale,'mode':'editable surface gradient',
                    'source_method':method,'note':'Preserves height and existing normal; screen derivatives and tessellation can differ between renderers.'}
            if scale < 0:
                height = self.binary("Subtract",self.constant(1),height)
                center = 1-center
            if method in ('DISPLACEMENT','BOTH'):
                channels["Displacement"] = height
                settings.update({"bEnableTessellation":"true", "bUsedWithNanite":"true", "DisplacementScaling":
                    f"(Magnitude={number(abs(scale)*100)},Center={number(center)})"})
            self.report["displacement"] = {"mode":"bump" if method=='BUMP' else 'nanite', "magnitude_cm":abs(scale)*100,"center":center,
                "source":key,"height_range":[0,1],"source_unit":"1 Blender unit = 1 metre",
                "requires":[] if method=='BUMP' else ["Nanite-enabled mesh", "r.Nanite.AllowTessellation=1 at startup", "r.Nanite.Tessellation=1"],
                "normal_behavior":"Source normal map is preserved; displaced geometry is not a guarantee of identical renderer normals."}
            self.report["translated"].append({"source":key,"type":displacement["type"],"socket":0})
        self.lines.append("  result = MaterialResult(" + ", ".join(f"{k}={v.ref}" for k,v in channels.items()) + ")")
        self.report["limitations"].append("Blender and Unreal lighting/BRDFs are not pixel-identical; consult the per-feature conversion report.")
        self.report['surface_policy']=self.surface_policy
        self.report["mode"] = "hybrid" if self.report["baked"] else "translated"
        settings_text = "\n".join("  "+key+" = "+value for key,value in settings.items())
        document = (f"gc 1 kind:material asset:{self.destination}\nparent: /Script/Engine.Material\n\n"
                    "settings {\n" + settings_text + ("\n" if settings_text else "") + "}\n\ngraph {\n" + "\n".join(self.lines) + "\n}\n\ncomments {\n}\n")
        return {"document":document, "textures":list(self.textures.values()), "report":self.report}
