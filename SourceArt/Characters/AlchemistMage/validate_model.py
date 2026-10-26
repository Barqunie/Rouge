"""Validate bind mesh, animation endpoints and export Unreal comparison samples."""
import bpy,json,math,struct
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
QA=OUT.parents[2]/'Saved/CodexModeling/mage_20261026'
stats=json.loads((OUT/'model_report.json').read_text())
clips=json.loads((OUT/'animation_report.json').read_text())['clips']
bpy.ops.wm.open_mainfile(filepath=str(OUT/'AlchemistMage_Animated.blend'))
rig=bpy.data.objects['Armature'];model=bpy.data.objects['SKM_AlchemistMage'];scene=bpy.context.scene
assert all(abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in model.data.vertices)
assert all(0<len(v.groups)<=4 for v in model.data.vertices)
assert all(p.area>1e-8 for p in model.data.polygons)
assert [b.name for b in rig.data.bones if not b.parent]==['root']
for side in ['l','r']:
    for parent,child in [('upperarm','lowerarm'),('lowerarm','hand'),('thigh','calf'),('calf','foot'),('foot','ball')]:
        assert (rig.data.bones[child+'_'+side].head_local-rig.data.bones[parent+'_'+side].tail_local).length<.001
expected={'reference_blender':{b.name:list(b.head_local) for b in rig.data.bones},'clips':{}}
for label,info in clips.items():
    rig.animation_data.action=bpy.data.actions[label];count=info['frames'];samples=[];matrices=[]
    for frame in range(1,count+1):
        scene.frame_set(frame);bpy.context.view_layer.update()
        heads={pb.name:list(pb.matrix.translation) for pb in rig.pose.bones}
        assert all(math.isfinite(v) for p in heads.values() for v in p)
        assert math.hypot(*heads['root'][:2])<.01
        for side in ['l','r']:
            for a,b in [('upperarm','lowerarm'),('lowerarm','hand'),('thigh','calf'),('calf','foot')]:
                length=(Vector(heads[a+'_'+side])-Vector(heads[b+'_'+side])).length
                assert abs(length-rig.data.bones[a+'_'+side].length)<.03,(label,frame,a,length)
        samples.append({'frame':frame,'time':(frame-1)/30,'heads_blender':heads})
        if frame in [1,count]:matrices.append({pb.name:pb.matrix.copy() for pb in rig.pose.bones})
    if info['loop']:
        assert max(abs(matrices[0][n][r][c]-matrices[1][n][r][c]) for n in matrices[0] for r in range(4) for c in range(4))<.001
    expected['clips'][label]={'duration':info['duration_seconds'],'samples':samples}
    scene.frame_set(19 if label=='AM_Cast' else 8)
    ev=model.evaluated_get(bpy.context.evaluated_depsgraph_get());m=ev.to_mesh()
    assert all(math.isfinite(c) for v in m.vertices for c in v.co)
    assert max(v.co.length for v in m.vertices)<240
    ev.to_mesh_clear()
(QA/'animation_expected.json').write_text(json.dumps(expected),encoding='utf-8')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'SKM_AlchemistMage_APose.fbx'))
arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(arms)==1 and len(meshes)==1
assert len(arms[0].data.bones)==stats['bones']
assert [b.name for b in arms[0].data.bones if not b.parent]==['root']
assert all(v.groups for v in meshes[0].data.vertices)
assert not bpy.data.actions
data=(OUT/'SKM_AlchemistMage_APose.glb').read_bytes();assert data[:4]==b'glTF'
size,kind=struct.unpack_from('<II',data,12);gltf=json.loads(data[20:20+size])
assert len(gltf['skins'])==1 and len(gltf['skins'][0]['joints'])==stats['bones']
assert all('JOINTS_0' in p['attributes'] and 'WEIGHTS_0' in p['attributes'] for m in gltf['meshes'] for p in m['primitives'])
animation_files={}
for label,info in clips.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/(label+'.fbx')))
    arms=[o for o in bpy.data.objects if o.type=='ARMATURE']
    assert len(arms)==1 and len(arms[0].data.bones)==stats['bones']
    assert len(bpy.data.actions)==1
    action=bpy.data.actions[0]
    assert abs((action.frame_range.y-action.frame_range.x)/bpy.context.scene.render.fps-info['duration_seconds'])<.04
    animation_files[label]='single take, skeleton roundtrip passed'
report={'fbx_roundtrip':'passed','glb_skin':'passed','weight_normalization':'passed','joint_continuity':'passed','root':'root','bones':stats['bones'],'animation_files':animation_files,'loop_endpoints':'identical','frames_checked':sum(c['frames'] for c in clips.values()),'max_influences':stats['max_influences']}
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('VALIDATION_COMPLETE',json.dumps(report),flush=True)
