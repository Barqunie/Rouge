import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
OUT=Path(__file__).resolve().parent
info=json.loads((OUT/'animation_report.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Gunner_Armed_Animated.blend'))
rig=bpy.data.objects['Armature'];model=bpy.data.objects['SKM_Gunner_Armed'];scene=bpy.context.scene
assert len(rig.data.bones)==98 and [b.name for b in rig.data.bones if not b.parent]==['root']
assert all(v.groups and len(v.groups)<=4 and abs(sum(g.weight for g in v.groups)-1)<1e-4 for v in model.data.vertices)
assert rig.data.bones['weapon_mount'].parent.name=='hand_r'
report={'frames_checked':0,'loop_matrix_error':{},'fbx':{},'grip_error_cm':info['max_wrist_grip_error_cm'],'bone_count':98,'root_motion':False}
ends={};samples={};max_root=0;max_lengths=0
for name,cfg in info['clips'].items():
    rig.animation_data.action=bpy.data.actions[name];matrices=[];samples[name]={}
    for frame in range(1,cfg['frames']+1):
        scene.frame_set(frame);bpy.context.view_layer.update();root=rig.pose.bones['root'].head
        max_root=max(max_root,math.hypot(root.x,root.y))
        for side in ['l','r']:
            for a,b in [('upperarm','lowerarm'),('lowerarm','hand')]:
                pb=rig.pose.bones[a+'_'+side];qb=rig.pose.bones[b+'_'+side]
                max_lengths=max(max_lengths,abs((pb.head-qb.head).length-pb.bone.length))
        if frame in [1,cfg['frames']]:matrices.append({p.name:p.matrix.copy() for p in rig.pose.bones})
        if frame in [1,cfg['frames']//2,cfg['frames']]:
            samples[name][frame]={p.name:list(rig.matrix_world@p.head) for p in rig.pose.bones}
            ev=model.evaluated_get(bpy.context.evaluated_depsgraph_get());m=ev.to_mesh()
            assert all(math.isfinite(c) for v in m.vertices for c in v.co)
            assert max(v.co.length for v in m.vertices)<280
            ev.to_mesh_clear()
        report['frames_checked']+=1
    ends[name]=matrices
    if cfg['loop']:
        error=max(abs(matrices[0][n][r][c]-matrices[-1][n][r][c]) for n in matrices[0] for r in range(4) for c in range(4));assert error<.002,(name,error)
        report['loop_matrix_error'][name]=error
assert max_root<.02 and max_lengths<.02
report['max_root_xy_cm']=max_root;report['max_arm_length_error_cm']=max_lengths
report['transitions']={}
for first,last in [('GUN_HoldIdle','GUN_FireStart'),('GUN_FireStart','GUN_FireLoop'),('GUN_FireLoop','GUN_FireEnd'),('GUN_FireEnd','GUN_HoldIdle')]:
    a=ends[first][-1];b=ends[last][0];error=max(abs(a[n][r][c]-b[n][r][c]) for n in a for r in range(4) for c in range(4))
    assert error<.002,(first,last,error);report['transitions'][first+' -> '+last]=error

for file in ['SKM_Gunner_Armed_APose.fbx']+[n+'.fbx' for n in info['clips']]:
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(OUT/file))
    arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
    assert len(arms)==1 and len(arms[0].data.bones)==98
    if file.startswith('SKM_'):
        assert len(meshes)==1 and len(meshes[0].data.polygons)==info['triangles']
        assert all(v.groups for v in meshes[0].data.vertices);assert not bpy.data.actions
    else:
        assert not meshes and len(bpy.data.actions)==1
        action=bpy.data.actions[0];cfg=info['clips'][Path(file).stem]
        assert abs((action.frame_range.y-action.frame_range.x)/bpy.context.scene.render.fps-cfg['duration_seconds'])<.01
        if 'Fire' in file:
            assert any('weapon_barrels' in fc.data_path for fc in action.fcurves)
        arm=arms[0];roundtrip_error=0
        for original_frame,positions in samples[Path(file).stem].items():
            f=action.frame_range.x+(original_frame-1)/30*bpy.context.scene.render.fps
            bpy.context.scene.frame_set(int(f),subframe=f-int(f));bpy.context.view_layer.update()
            factor=bpy.context.scene.unit_settings.scale_length*100
            roundtrip_error=max(roundtrip_error,max(((arm.matrix_world@arm.pose.bones[n].head)*factor-Vector(p)).length for n,p in positions.items()))
        assert roundtrip_error<.05,(file,roundtrip_error)
        report.setdefault('fbx_pose_error_cm',{})[file]=roundtrip_error
    report['fbx'][file]='passed'

weapon_info=json.loads((OUT/'weapon_animation_report.json').read_text())
for file in ['SKM_Minigun_Rigged.fbx']+[n+'.fbx' for n in weapon_info]:
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(OUT/file))
    arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];assert len(arms)==1 and len(arms[0].data.bones)==5
    if file.startswith('SKM_'):assert len([o for o in bpy.data.objects if o.type=='MESH'])==1
    else:
        assert len(bpy.data.actions)==1
        action=bpy.data.actions[0];cfg=weapon_info[Path(file).stem]
        assert abs((action.frame_range.y-action.frame_range.x)/bpy.context.scene.render.fps-cfg['duration'])<.01
    report['fbx'][file]='passed'
report['Unreal_import']='not performed'
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('ARMED_VALIDATION_COMPLETE',json.dumps(report),flush=True)
