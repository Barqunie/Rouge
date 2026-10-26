"""Check geometry and roundtrip skinning; bent poses are internal QA only."""
import bpy
import json
import math
import struct
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parent
QA=OUT.parents[2]/'Saved/CodexModeling/bulk_20261026';QA.mkdir(parents=True,exist_ok=True)
stats=json.loads((OUT/'model_report.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Bulk_APose.blend'))
model=bpy.data.objects['SKM_Bulk'];rig=bpy.data.objects['Armature']
assert all(abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in model.data.vertices)
assert all(0<len(v.groups)<=4 for v in model.data.vertices)
assert all(p.area>1e-8 for p in model.data.polygons)
for side in ['l','r']:
    assert abs(rig.data.bones['upperarm_'+side].length-stats['upperarm_cm'])<.01
    assert abs(rig.data.bones['lowerarm_'+side].length-stats['forearm_cm'])<.01
    for parent,child in [('upperarm','lowerarm'),('lowerarm','hand'),('thigh','calf'),('calf','foot'),('foot','ball')]:
        assert (rig.data.bones[child+'_'+side].head_local-rig.data.bones[parent+'_'+side].tail_local).length<.001
for name,axis,angle in [('upperarm_l',2,-20),('upperarm_r',2,12),('lowerarm_l',0,-65),('lowerarm_r',0,-50),('thigh_l',0,-25),('calf_l',0,52),('calf_r',0,12),('apron_01',0,10)]:
    pb=rig.pose.bones[name];pb.rotation_mode='XYZ';pb.rotation_euler[axis]=math.radians(angle)
for side in ['l','r']:
    for f in ['index','middle','ring','pinky']:
        for i in [1,2,3]:
            pb=rig.pose.bones[f'{f}_{i:02}_{side}'];pb.rotation_mode='XYZ';pb.rotation_euler[0]=math.radians(-35)
bpy.context.view_layer.update()
deps=bpy.context.evaluated_depsgraph_get();evaluated=model.evaluated_get(deps);posed=evaluated.to_mesh()
assert all(math.isfinite(c) for v in posed.vertices for c in v.co)
group=model.vertex_groups['tank'].index
ids=[v.index for v in model.data.vertices if len(v.groups)==1 and v.groups[0].group==group]
assert len(ids)>50
for i in ids[::11]:
    expected=rig.pose.bones['tank'].matrix@rig.data.bones['tank'].matrix_local.inverted()@model.data.vertices[i].co
    assert (expected-posed.vertices[i].co).length<.003
evaluated.to_mesh_clear()
bpy.context.scene.render.filepath=str(QA/'Preview_RigTest.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'SKM_Bulk_APose.fbx'))
arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(arms)==1 and len(meshes)==1
assert [b.name for b in arms[0].data.bones if not b.parent]==['root']
assert len(arms[0].data.bones)==stats['bones']
assert any(m.type=='ARMATURE' for m in meshes[0].modifiers)
assert all(v.groups for v in meshes[0].data.vertices)
assert not bpy.data.actions,'No animation takes requested for the delivered FBX'
data=(OUT/'SKM_Bulk_APose.glb').read_bytes();assert data[:4]==b'glTF'
size,kind=struct.unpack_from('<II',data,12);gltf=json.loads(data[20:20+size])
assert len(gltf['skins'])==1 and len(gltf['skins'][0]['joints'])==stats['bones']
assert all('JOINTS_0' in p['attributes'] and 'WEIGHTS_0' in p['attributes'] for m in gltf['meshes'] for p in m['primitives'])
assert not gltf.get('animations')
report={'fbx_roundtrip':'passed','glb_skin':'passed','root':'root','bone_count':stats['bones'],'weight_normalization':'passed','max_influences':stats['max_influences'],'joint_continuity':'passed','tank_rigidity':'passed','animation_takes':0,'posed_QA':'finite; internal preview saved outside delivery'}
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('VALIDATION_COMPLETE',json.dumps(report),flush=True)
