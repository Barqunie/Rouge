"""Validate delivered skins, joints, accessories and a deliberately bent test pose."""
import bpy
import json
import math
import struct
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parent
stats=json.loads((OUT/'model_report.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Needle_APose.blend'))
model=bpy.data.objects['SKM_Needle'];rig=bpy.data.objects['Armature']
assert all(abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in model.data.vertices)
assert all(0<len(v.groups)<=4 for v in model.data.vertices)
assert all(p.area>1e-8 for p in model.data.polygons)
for side in ['l','r']:
    assert abs(rig.data.bones['upperarm_'+side].length-stats['upperarm_cm'])<.01
    assert abs(rig.data.bones['lowerarm_'+side].length-stats['forearm_cm'])<.01
    for parent,child in [('upperarm','lowerarm'),('lowerarm','hand'),('thigh','calf'),('calf','foot'),('foot','ball')]:
        assert (rig.data.bones[child+'_'+side].head_local-rig.data.bones[parent+'_'+side].tail_local).length<.001
    assert rig.data.bones['weapon_'+side].use_deform
for name,axis,angle in [('upperarm_l',2,-26),('upperarm_r',2,22),('lowerarm_l',0,-67),('lowerarm_r',0,-52),('thigh_l',0,-27),('calf_l',0,56),('calf_r',0,12),('cape_l_02',0,18),('cape_c_02',0,12),('cape_r_02',0,15)]:
    pb=rig.pose.bones[name];pb.rotation_mode='XYZ';pb.rotation_euler[axis]=math.radians(angle)
for side in ['l','r']:
    for finger in ['index','middle','ring','pinky']:
        for i in [1,2,3]:
            pb=rig.pose.bones[f'{finger}_{i:02}_{side}'];pb.rotation_mode='XYZ';pb.rotation_euler[0]=math.radians(-25)
    bpy.data.objects['Dagger_'+side].hide_render=False
bpy.context.view_layer.update()
deps=bpy.context.evaluated_depsgraph_get()
for name in ['SKM_Needle','Dagger_l','Dagger_r']:
    evaluated=bpy.data.objects[name].evaluated_get(deps);posed=evaluated.to_mesh()
    assert all(math.isfinite(c) for v in posed.vertices for c in v.co)
    if name.startswith('Dagger_'):
        side=name[-1];b=rig.data.bones['weapon_'+side];pb=rig.pose.bones['weapon_'+side]
        predicted=pb.matrix@b.matrix_local.inverted()@bpy.data.objects[name].data.vertices[0].co
        assert (predicted-posed.vertices[0].co).length<.002,'Weapon does not follow the hand'
    evaluated.to_mesh_clear()
bpy.context.scene.render.filepath=str(OUT/'Preview_RigTest.png')
bpy.ops.render.render(write_still=True)
report={'weights':'normalized, no unweighted vertices','max_influences':stats['max_influences'],'joint_continuity':'passed','cape_and_weapons':'pose tested; weapons follow hands','fbx_roundtrips':{}}
for filename,count in [('SKM_Needle_APose.fbx',1),('SKM_Needle_Equipped_APose.fbx',3),('SM_Needle_Dagger.fbx',1)]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/filename))
    arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
    assert len(meshes)==count,(filename,len(meshes))
    if filename.startswith('SKM'):
        assert len(arms)==1
        assert [b.name for b in arms[0].data.bones if not b.parent]==['root']
        assert len(arms[0].data.bones)==stats['bones']
        assert all(any(m.type=='ARMATURE' for m in obj.modifiers) for obj in meshes)
        assert all(v.groups for obj in meshes for v in obj.data.vertices)
    else:assert len(arms)==0
    report['fbx_roundtrips'][filename]={'meshes':len(meshes),'armatures':len(arms),'passed':True}
data=(OUT/'SKM_Needle_APose.glb').read_bytes()
assert data[:4]==b'glTF'
size,kind=struct.unpack_from('<II',data,12)
gltf=json.loads(data[20:20+size]);assert len(gltf['skins'])==1
assert len(gltf['skins'][0]['joints'])==stats['bones']
assert all('JOINTS_0' in p['attributes'] and 'WEIGHTS_0' in p['attributes'] for m in gltf['meshes'] for p in m['primitives'])
report['glb_skin']='passed'
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('VALIDATION_COMPLETE',json.dumps(report),flush=True)
