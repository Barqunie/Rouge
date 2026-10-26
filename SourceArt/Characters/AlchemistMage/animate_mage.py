"""Bake verified UE retarget data and an authored casting action to separate FBXs."""
import bpy, json, math
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion
OUT=Path(__file__).resolve().parent
QA=OUT.parents[2]/'Saved/CodexModeling/mage_20261026'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'AlchemistMage_APose.blend'))
rig=bpy.data.objects['Armature'];model=bpy.data.objects['SKM_AlchemistMage'];scene=bpy.context.scene
scene.render.fps=30;scene.render.fps_base=1
scene.render.resolution_x=760;scene.render.resolution_y=760;scene.cycles.samples=18
rig.animation_data_create()
for pb in rig.pose.bones:pb.rotation_mode='QUATERNION'
def transform(t):return Matrix.LocRotScale(Vector(t['translation']),Quaternion(t['rotation']),Vector(t['scale']))
idle=json.loads((QA/'Retarget_AM_MM_Idle_Verified.json').read_text())
names=[b.name for b in rig.data.bones if b.name in idle['reference']]
ue=np.array([idle['reference'][n]['translation']+[1.] for n in names]);bl=np.array([list(rig.data.bones[n].head_local) for n in names])
coeff=np.linalg.lstsq(ue,bl,rcond=None)[0]
fit_error=float(np.max(np.linalg.norm(ue@coeff-bl,axis=1)))
assert fit_error<.01,fit_error
C=Matrix.Identity(4)
for row in range(3):
    for col in range(4):C[row][col]=float(coeff[col,row])
Ci=C.inverted();refs={n:transform(idle['reference'][n]) for n in names}
report={'coordinate_fit_error_cm':fit_error,'clips':{}}
def pose_from_sample(sample):
    desired={n:C@transform(sample['bones'][n])@refs[n].inverted()@Ci@rig.data.bones[n].matrix_local for n in names}
    root_delta=desired['root'].translation-rig.data.bones['root'].matrix_local.translation
    root_delta.z=0
    for mat in desired.values():mat.translation-=root_delta
    for n in names:
        pb=rig.pose.bones[n];rest=pb.bone.matrix_local
        pb.matrix_basis=rest.inverted()@pb.parent.bone.matrix_local@desired[pb.parent.name].inverted()@desired[n] if pb.parent else rest.inverted()@desired[n]
    bpy.context.view_layer.update()
    err=max((rig.pose.bones[n].matrix.translation-desired[n].translation).length for n in names)
    assert err<.02,err
    return err
def rotation_object_axis(name,axis,angle,add=False):
    pb=rig.pose.bones[name]
    local_axis=pb.bone.matrix_local.to_quaternion().inverted()@Vector(axis)
    q=Quaternion(local_axis,math.radians(angle))
    pb.rotation_quaternion=pb.rotation_quaternion@q if add else q
def secondary(phase,run=False,cast=0):
    for side,offset in [('l',0),('r',math.pi)]:
        thigh=rig.pose.bones['thigh_'+side]
        delta=thigh.rotation_quaternion.to_euler('XYZ').x
        # Follow the thigh gently while keeping the split tails outside the legs.
        for i in range(1,4):
            rotation_object_axis(f'coat_side_{side}_{i:02}',(1,0,0),math.degrees(delta)*(.50 if i==1 else .10)+(5 if run else 2)*math.sin(phase+offset-i*.45)+cast*3)
            rotation_object_axis(f'coat_back_{side}_{i:02}',(1,0,0),(8 if i==1 else 3)+(7 if run else 3)*math.sin(phase+offset-i*.5)+cast*6)
            rotation_object_axis(f'hair_{side}_{i:02}',(1,0,0),(4 if run else 1)+3*math.sin(phase+offset*.25-i*.6)+cast*6)
            rotation_object_axis(f'hair_{side}_{i:02}',(0,0,1),2*math.sin(phase-i*.5)+cast*2,True)
def sync_helpers():
    bpy.context.view_layer.update()
    for dst,src in [('ik_hand_gun','hand_r'),('ik_hand_l','hand_l'),('ik_hand_r','hand_r'),('ik_foot_l','foot_l'),('ik_foot_r','foot_r')]:
        rig.pose.bones[dst].matrix=rig.pose.bones[src].matrix
        bpy.context.view_layer.update()
def key(frame):
    for pb in rig.pose.bones:
        pb.keyframe_insert('location',frame=frame,group=pb.name)
        pb.keyframe_insert('rotation_quaternion',frame=frame,group=pb.name)
        pb.keyframe_insert('scale',frame=frame,group=pb.name)
def new_action(name):
    rig.animation_data.action=None
    for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
    a=bpy.data.actions.new(name);a.use_fake_user=True;rig.animation_data.action=a
    return a
def export_action(action,count,loop):
    rig.animation_data.action=action;scene.frame_start=1;scene.frame_end=count;scene.frame_set(1)
    for curve in action.fcurves:
        for k in curve.keyframe_points:k.interpolation='LINEAR'
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
    path=OUT/(action.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
    roots=[]
    for frame in range(1,count+1):
        scene.frame_set(frame);roots.append(list(rig.pose.bones['root'].matrix.translation))
    assert max(abs(v[i]) for v in roots for i in [0,1])<.01
    report['clips'][action.name]={'frames':count,'fps':30,'duration_seconds':(count-1)/30,'loop':loop,'in_place':True,'file':path.name}
    print('ANIMATION_EXPORTED',action.name,count,flush=True)
for suffix,label in [('MM_Idle','Idle'),('MF_Unarmed_Walk_Fwd','Walk'),('MF_Unarmed_Jog_Fwd','Run')]:
    data=json.loads((QA/f'Retarget_AM_{suffix}_Verified.json').read_text())
    samples=data['samples'];a=new_action('AM_'+label);count=len(samples)
    for i,sample in enumerate(samples):
        scene.frame_set(i+1)
        # Exact endpoint equality gives a seamless looping animation.
        pose_from_sample(samples[0] if i==count-1 else sample)
        secondary(math.tau*i/(count-1),run=label=='Run')
        sync_helpers();key(i+1)
    export_action(a,count,True)

# Author a one-shot cast using a planted stance and analytic two-bone arm IK.
def aim_bone(name,start,end):
    pb=rig.pose.bones[name];old=pb.matrix.to_quaternion()
    direction=pb.matrix.to_3x3()@Vector((0,1,0))
    q=direction.rotation_difference((end-start).normalized())@old
    pb.matrix=Matrix.LocRotScale(start,q,Vector((1,1,1)))
    bpy.context.view_layer.update()
def solve_arm(side,target,pole,hand_dir):
    upper=rig.pose.bones['upperarm_'+side];lower=rig.pose.bones['lowerarm_'+side]
    start=upper.matrix.translation.copy();end=Vector(target)*100;pole=Vector(pole)*100
    l1=upper.bone.length;l2=lower.bone.length;direction=end-start;d=min(direction.length,l1+l2-.2)
    unit=direction.normalized();end=start+unit*d
    across=pole-start-unit*(pole-start).dot(unit);across.normalize()
    a=(l1*l1-l2*l2+d*d)/(2*d);h=math.sqrt(max(0,l1*l1-a*a));mid=start+unit*a+across*h
    aim_bone(upper.name,start,mid);aim_bone(lower.name,mid,end)
    # Map the hand's rest finger axis / palm normal into a deliberate open-palm frame.
    hand=rig.pose.bones['hand_'+side];old_y=(hand.bone.tail_local-hand.bone.head_local).normalized()
    old_z=Vector((0,-1,0));old_x=old_y.cross(old_z).normalized();old_z=old_x.cross(old_y).normalized()
    new_y=Vector(hand_dir).normalized();new_z=Vector((0,-1,0));new_x=new_y.cross(new_z).normalized();new_z=new_x.cross(new_y).normalized()
    old_basis=Matrix((old_x,old_y,old_z)).transposed();new_basis=Matrix((new_x,new_y,new_z)).transposed()
    q=(new_basis@old_basis.inverted()).to_quaternion()@hand.bone.matrix_local.to_quaternion()
    hand.matrix=Matrix.LocRotScale(end,q,Vector((1,1,1)))
    bpy.context.view_layer.update()
def interpolate(keys,t):
    for j in range(len(keys)-1):
        ta,va=keys[j];tb,vb=keys[j+1]
        if t<=tb:
            f=max(0,min(1,(t-ta)/(tb-ta)));f=f*f*(3-2*f)
            if isinstance(va,(tuple,list)):return tuple(a+(b-a)*f for a,b in zip(va,vb))
            return va+(vb-va)*f
    return keys[-1][1]
a=new_action('AM_Cast');count=40
pose_from_sample(idle['samples'][0])
idle_right=tuple(rig.pose.bones['hand_r'].matrix.translation/100)
idle_left=tuple(rig.pose.bones['hand_l'].matrix.translation/100)
for i in range(count):
    frame=i+1;t=i/30;scene.frame_set(frame);pose_from_sample(idle['samples'][0])
    power=interpolate([(0,0),(.25,.6),(.45,1),(.6,-.8),(.76,-.45),(1.3,0)],t)
    rotation_object_axis('spine_02',(0,0,1),power*10,True)
    rotation_object_axis('spine_04',(0,0,1),power*11,True)
    rotation_object_axis('spine_03',(1,0,0),power*-4,True)
    rotation_object_axis('neck_02',(0,0,1),-power*14,True)
    bpy.context.view_layer.update()
    right=interpolate([(0,idle_right),(.20,(-.32,-.11,1.33)),(.43,(-.29,.11,1.49)),(.60,(-.17,-.515,1.425)),(.73,(-.18,-.49,1.42)),(1.04,(-.29,-.18,1.27)),(1.3,idle_right)],t)
    left=interpolate([(0,idle_left),(.36,(.29,-.10,1.13)),(.60,(.32,-.12,1.10)),(.9,(.29,-.11,1.12)),(1.3,idle_left)],t)
    handdir=interpolate([(0,(-.2,-.12,-1)),(.25,(.1,-.4,.8)),(.43,(.1,-.5,.7)),(.60,(0,-.15,1)),(.78,(0,-.25,.85)),(1.3,(-.2,-.12,-1))],t)
    solve_arm('r',right,(-.64,-.02,1.32),handdir)
    solve_arm('l',left,(.55,.02,1.23),(.2,-.2,-.9))
    curl=interpolate([(0,18),(.25,45),(.43,70),(.60,4),(.73,8),(1.3,18)],t)
    for side in ['l','r']:
        for finger in ['index','middle','ring','pinky']:
            for j in range(1,4):
                pb=rig.pose.bones[f'{finger}_{j:02}_{side}'];pb.rotation_quaternion=Quaternion((1,0,0),math.radians(-curl*(1 if side=='r' else .45)))
    secondary(t*math.tau,cast=power)
    sync_helpers();key(frame)
# Blend the last six frames exactly to the retargeted idle, avoiding an arm snap.
pose_from_sample(idle['samples'][0]);secondary(0);sync_helpers()
rest_end={pb.name:(pb.location.copy(),pb.rotation_quaternion.copy(),pb.scale.copy()) for pb in rig.pose.bones}
scene.frame_set(6)
anticipation={pb.name:(pb.location.copy(),pb.rotation_quaternion.copy(),pb.scale.copy()) for pb in rig.pose.bones}
for frame in range(1,6):
    scene.frame_set(frame);u=(frame-1)/5;u=u*u*(3-2*u)
    for pb in rig.pose.bones:
        l,q,s=rest_end[pb.name];le,qe,se=anticipation[pb.name]
        pb.location=l.lerp(le,u);pb.rotation_quaternion=q.slerp(qe,u);pb.scale=s.lerp(se,u)
    key(frame)
scene.frame_set(32)
recovery={pb.name:(pb.location.copy(),pb.rotation_quaternion.copy(),pb.scale.copy()) for pb in rig.pose.bones}
for frame in range(33,41):
    scene.frame_set(frame);u=(frame-32)/8;u=u*u*(3-2*u)
    for pb in rig.pose.bones:
        l,q,s=recovery[pb.name];le,qe,se=rest_end[pb.name]
        pb.location=l.lerp(le,u);pb.rotation_quaternion=q.slerp(qe,u);pb.scale=s.lerp(se,u)
    key(frame)
export_action(a,count,False)
report['clips']['AM_Cast']['release_frame']=19
report['clips']['AM_Cast']['release_seconds']=.60
report['clips']['AM_Cast']['note']='Right palm forward; anticipation, release, follow-through, recovery. No projectile/VFX gameplay logic.'
(OUT/'animation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
rig.animation_data.action=bpy.data.actions['AM_Idle'];scene.frame_start=1;scene.frame_end=len(idle['samples']);scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'AlchemistMage_Animated.blend'))
for label,frame in [('Walk',8),('Run',7),('Cast',19)]:
    rig.animation_data.action=bpy.data.actions['AM_'+label];scene.frame_set(frame)
    scene.render.filepath=str(OUT/f'Preview_{label}.png');bpy.ops.render.render(write_still=True)
print('ANIMATIONS_COMPLETE',json.dumps(report),flush=True)
