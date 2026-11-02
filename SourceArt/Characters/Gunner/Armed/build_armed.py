"""Two-handed minigun poses, baked locomotion/fire clips and separate rotor rig."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
OUT=Path(__file__).resolve().parent;BASE=OUT.parent;MAGE=BASE.parent/'AlchemistMage'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'Gunner_APose.blend'))
rig=bpy.data.objects['Armature'];original=bpy.data.objects['SKM_Gunner'];scene=bpy.context.scene
scene.render.fps=30;scene.render.fps_base=1;scene.cycles.samples=28
scene.render.resolution_x=1100;scene.render.resolution_y=1300
for p in rig.pose.bones:p.rotation_mode='QUATERNION'
rig.animation_data_create()
receiver=bpy.data.objects['SM_Gunner_Minigun_Body'];barrels=bpy.data.objects['SM_Gunner_Minigun_Barrels']
for ob in [receiver,barrels]:ob.hide_set(False);ob.hide_render=True
with bpy.data.libraries.load(str(MAGE/'AlchemistMage_Animated.blend'),link=False) as (src,dst):dst.actions=['AM_Idle','AM_Walk']
source_actions={a.name:a for a in dst.actions}
base_names=[b.name for b in rig.data.bones]

def capture_source(name,count,full=True):
    action=source_actions[name];rig.animation_data.action=action;poses=[]
    for i in range(count):
        f=action.frame_range.x+(action.frame_range.y-action.frame_range.x)*i/(count-1) if full else action.frame_range.x
        frame=int(f);scene.frame_set(frame,subframe=f-frame)
        poses.append({p.name:p.matrix_basis.copy() for p in rig.pose.bones})
    if full:poses[-1]={n:m.copy() for n,m in poses[0].items()}
    return poses
walk_base=capture_source('AM_Walk',46);idle_base=capture_source('AM_Idle',61)
steady=idle_base[0]
rig.animation_data.action=None

def apply_base(pose):
    for p in rig.pose.bones:p.matrix_basis=pose.get(p.name,Matrix.Identity(4)).copy()
    bpy.context.view_layer.update()
apply_base(steady);pelvis0=rig.pose.bones['pelvis'].head.copy()

def rot_axis(name,axis,degrees):
    p=rig.pose.bones[name];local=p.bone.matrix_local.to_quaternion().inverted()@Vector(axis)
    p.rotation_quaternion=p.rotation_quaternion@Quaternion(local,math.radians(degrees))

def aim_bone(name,start,end):
    pb=rig.pose.bones[name];oldq=pb.matrix.to_quaternion();oldy=oldq@Vector((0,1,0))
    q=oldy.rotation_difference((end-start).normalized())@oldq
    pb.matrix=Matrix.LocRotScale(start,q,Vector((1,1,1)));bpy.context.view_layer.update()

def hand_rotation(side,finger,palm):
    b=rig.data.bones['hand_'+side];oy=(b.tail_local-b.head_local).normalized();oz=Vector((0,1,0));ox=oy.cross(oz).normalized();oz=ox.cross(oy).normalized()
    ny=Vector(finger).normalized();nz=Vector(palm).normalized();nx=ny.cross(nz).normalized();nz=nx.cross(ny).normalized()
    old=Matrix((ox,oy,oz)).transposed();new=Matrix((nx,ny,nz)).transposed()
    return (new@old.transposed()).to_quaternion()@b.matrix_local.to_quaternion()

# Local grip coordinates in centimetres; wrist offsets account for the palm thickness.
FINGER={'r':Vector((0,-1,0)),'l':Vector((-1,0,0))}
PALM={'r':Vector((1,0,0)),'l':Vector((0,0,-1))}
GRIP={'r':Vector((-17,8,25)),'l':Vector((0,-7,28.4))}
WRIST={s:GRIP[s]-FINGER[s]*7.3-PALM[s]*3.0 for s in ['r','l']}
HAND_LOCAL={s:Matrix.LocRotScale(WRIST[s],hand_rotation(s,FINGER[s],PALM[s]),Vector((1,1,1))) for s in ['r','l']}
max_reach_error=0.0
def solve_arm(side,G):
    global max_reach_error
    u=rig.pose.bones['upperarm_'+side];l=rig.pose.bones['lowerarm_'+side]
    start=u.head.copy();target=G@WRIST[side];dvec=target-start;d=dvec.length
    assert d<u.bone.length+l.bone.length-.05,(side,d,u.bone.length+l.bone.length)
    unit=dvec.normalized();pole=Vector((32,13,110) if side=='l' else (-27,24,106))
    v=pole-start-unit*(pole-start).dot(unit);v.normalize()
    a=(u.bone.length**2-l.bone.length**2+d*d)/(2*d);h=math.sqrt(max(0,u.bone.length**2-a*a));mid=start+unit*a+v*h
    aim_bone(u.name,start,mid);aim_bone(l.name,mid,target)
    hand=rig.pose.bones['hand_'+side];hand.matrix=G@HAND_LOCAL[side];bpy.context.view_layer.update()
    max_reach_error=max(max_reach_error,(l.tail-hand.head).length)
    # Finger curls are around each rest finger's palm-facing axis.
    for finger in ['index','middle','ring','pinky']:
        amounts=[48,65,38] if not(side=='r' and finger=='index') else [35,50,32]
        for j,angle in enumerate(amounts,1):
            pb=rig.pose.bones[f'{finger}_{j:02}_{side}'];direction=(pb.bone.tail_local-pb.bone.head_local).normalized();axis=direction.cross(Vector((0,1,0))).normalized()
            local=pb.bone.matrix_local.to_quaternion().inverted()@axis;pb.rotation_quaternion=Quaternion(local,math.radians(angle))
    bpy.context.view_layer.update()
    # Oppose the thumb along the handle rather than leaving it splayed outward.
    direction=G.to_3x3()@FINGER[side];palm=G.to_3x3()@PALM[side]
    for j,(a,b) in enumerate([(1,.45),(.70,.75),(.20,1.0)],1):
        pb=rig.pose.bones[f'thumb_{j:02}_{side}'];start=pb.head.copy();axis=(direction*a+palm*b).normalized();aim_bone(pb.name,start,start+axis*pb.bone.length)

def gun_transform(phase,power=0,walking=False):
    delta=rig.pose.bones['pelvis'].head-pelvis0
    recoil=power*.55*(1-math.cos(phase*(12 if walking else 8)))
    location=Vector((-5,-22,85))+Vector((delta.x*.65,delta.y*.6+recoil,delta.z*.8))
    yaw=(.012*math.sin(phase) if walking else .027*power*math.sin(phase))
    # Low carry: muzzle down at rest, hip-level horizontal aim while firing.
    pitch=math.radians(18)*(1-power)+(.012*math.sin(phase*2) if walking else .004*math.sin(phase))
    pitch-=power*.012*(1-math.cos(phase*(12 if walking else 8)))
    q=Quaternion((0,0,1),yaw)@Quaternion((1,0,0),pitch)
    return Matrix.LocRotScale(location,q,Vector((1,1,1)))

def pose_body(base,phase,power=0,walking=False):
    apply_base(base)
    rot_axis('spine_02',(1,0,0),3+power*1.5);rot_axis('spine_04',(1,0,0),2+power)
    rot_axis('neck_02',(1,0,0),-3-power)
    if power:rot_axis('spine_03',(0,0,1),power*1.2*math.sin(phase))
    for side in ['l','r']:
        for j in [1,2,3]:
            rot_axis(f'hair_{side}_{j:02}',(1,0,0),(2 if walking else 1)*math.sin(phase-j*.55))
    bpy.context.view_layer.update();G=gun_transform(phase,power,walking)
    solve_arm('r',G);solve_arm('l',G)
    for dst,src in [('ik_hand_gun','hand_r'),('ik_hand_l','hand_l'),('ik_hand_r','hand_r'),('ik_foot_l','foot_l'),('ik_foot_r','foot_r')]:
        rig.pose.bones[dst].matrix=rig.pose.bones[src].matrix;bpy.context.view_layer.update()
    return G

G0=pose_body(steady,0);H0=rig.pose.bones['hand_r'].matrix.copy();attachment=H0.inverted()@G0
R=rig.data.bones['hand_r'].matrix_local;Grest=R@attachment
for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='EDIT')
def add_weapon_bone(name,a,b,parent):
    bone=rig.data.edit_bones.new(name);bone.head=Grest@Vector(a);bone.tail=Grest@Vector(b);bone.parent=rig.data.edit_bones[parent];bone.use_deform=name in ['weapon_mount','weapon_barrels'];return bone
add_weapon_bone('weapon_mount',(0,0,0),(0,0,10),'hand_r')
add_weapon_bone('weapon_barrels',(0,-42,2.5),(0,-95,2.5),'weapon_mount')
add_weapon_bone('weapon_muzzle',(0,-101.8,2.5),(0,-111.8,2.5),'weapon_mount')
support=add_weapon_bone('weapon_grip_l',WRIST['l'],WRIST['l']+FINGER['l']*8.3,'weapon_mount');support.matrix=Grest@HAND_LOCAL['l'];support.length=8.3
bpy.ops.object.mode_set(mode='OBJECT')
for pb in rig.pose.bones:pb.rotation_mode='QUATERNION'
mount_rest=rig.data.bones['weapon_mount'].matrix_local.copy()
mount_to_gun=mount_rest.inverted()@Grest

def clone_mesh(ob,name,transform=None,group=None):
    copy=ob.copy();copy.data=ob.data.copy();copy.name=name;scene.collection.objects.link(copy);copy.hide_render=False;copy.hide_set(False)
    if transform is not None:
        wm=ob.matrix_world.copy();copy.parent=None;copy.matrix_world=Matrix.Identity(4)
        for v in copy.data.vertices:v.co=transform@wm@v.co
    if group:
        for vg in list(copy.vertex_groups):copy.vertex_groups.remove(vg)
        vg=copy.vertex_groups.new(name=group);vg.add(list(range(len(copy.data.vertices))),1,'REPLACE')
    return copy
char=clone_mesh(original,'SKM_Gunner_Armed');body=clone_mesh(receiver,'Armed_weapon_body',Grest,'weapon_mount');rotor=clone_mesh(barrels,'Armed_weapon_barrels',Grest,'weapon_barrels')
original.hide_render=True;original.hide_set(True)
bpy.ops.object.select_all(action='DESELECT')
for ob in [char,body,rotor]:ob.select_set(True)
bpy.context.view_layer.objects.active=char;bpy.ops.object.join();char=bpy.context.object;char.name='SKM_Gunner_Armed';rig.data.name='SK_Gunner_Armed'

def export_fbx(path,objects,animation=False):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:ob.hide_set(False);ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=animation,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0,path_mode='COPY',embed_textures=True)
export_fbx(OUT/'SKM_Gunner_Armed_APose.fbx',[rig,char])

report={'bones':len(rig.data.bones),'triangles':len(char.data.polygons),'weapon_parent':'hand_r','carry_style':'low hip carry; both hands near belt; muzzle lowered 18 degrees while idle/walking, horizontal hip fire','gun_axis_height_cm':85,'attachment_hand_to_weapon_blender_cm':[list(r) for r in attachment],'grip_wrist_local_cm':{s:list(v) for s,v in WRIST.items()},'clips':{},'rpm':480,'in_place':True}
configs=[('GUN_HoldIdle',61,True,'idle'),('GUN_Walk',46,True,'walk'),('GUN_FireStart',16,False,'start'),('GUN_FireLoop',31,True,'fire'),('GUN_WalkFire',46,True,'walkfire'),('GUN_FireEnd',16,False,'end')]
max_grip_error=0.;max_attach_error=0.
for name,count,loop,kind in configs:
    action=bpy.data.actions.new(name);action.use_fake_user=True;rig.animation_data.action=action
    for i in range(count):
        frame=i+1;scene.frame_set(frame);u=i/(count-1);t=i/30;phase=math.tau*u;walking=kind in ['walk','walkfire']
        if kind=='start':power=u*u*(3-2*u);angle=math.tau*8*t*t/(2*.5)
        elif kind=='end':power=1-u*u*(3-2*u);angle=math.tau*8*(t-t*t/(2*.5))
        elif kind in ['fire','walkfire']:power=1;angle=math.tau*8*t
        else:power=0;angle=0
        base=walk_base[i] if walking else (idle_base[i] if kind=='idle' else steady)
        G=pose_body(base,phase,power,walking)
        rig.pose.bones['weapon_barrels'].rotation_quaternion=Quaternion((0,1,0),angle)
        bpy.context.view_layer.update()
        actual=rig.pose.bones['weapon_mount'].matrix@mount_to_gun
        max_attach_error=max(max_attach_error,max(abs(actual[r][c]-G[r][c]) for r in range(4) for c in range(4)))
        for side in ['l','r']:max_grip_error=max(max_grip_error,(rig.pose.bones['hand_'+side].head-actual@WRIST[side]).length)
        for pb in rig.pose.bones:
            pb.keyframe_insert('location',frame=frame,group=pb.name);pb.keyframe_insert('rotation_quaternion',frame=frame,group=pb.name);pb.keyframe_insert('scale',frame=frame,group=pb.name)
    for fc in action.fcurves:
        for k in fc.keyframe_points:k.interpolation='LINEAR'
    scene.frame_start=1;scene.frame_end=count;scene.frame_set(1)
    export_fbx(OUT/(name+'.fbx'),[rig],True)
    report['clips'][name]={'frames':count,'fps':30,'duration_seconds':(count-1)/30,'loop':loop,'root_motion':False,'weapon_and_barrel_animation_included':True}
    print('ARMED_CLIP_EXPORTED',name,flush=True)
report['max_wrist_grip_error_cm']=max_grip_error;report['max_attachment_matrix_error']=max_attach_error;report['max_arm_chain_gap_cm']=max_reach_error
assert max_grip_error<.02 and max_attach_error<.02 and max_reach_error<.02,report
(OUT/'animation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
rig.animation_data.action=bpy.data.actions['GUN_HoldIdle'];scene.frame_start=1;scene.frame_end=61;scene.frame_set(1)
camera=scene.camera
def point_at(ob,target):ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
camera.location=(285,-600,238);point_at(camera,(-2,-20,98));camera.data.ortho_scale=221
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Gunner_Armed_Animated.blend'))
for label,action,frame in [('Hold','GUN_HoldIdle',1),('Walk','GUN_Walk',12),('Fire','GUN_FireLoop',5),('WalkFire','GUN_WalkFire',12)]:
    rig.animation_data.action=bpy.data.actions[action];scene.frame_set(frame);scene.render.filepath=str(OUT/f'Preview_{label}.png');bpy.ops.render.render(write_still=True)
    print('ARMED_PREVIEW_READY',label,flush=True)
print('ARMED_BUILD_COMPLETE',json.dumps(report),flush=True)
