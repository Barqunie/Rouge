"""Independent minigun skeletal prop and start/loop/stop rotor animations."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
OUT=Path(__file__).resolve().parent;BASE=OUT.parent
bpy.ops.wm.open_mainfile(filepath=str(BASE/'Gunner_APose.blend'))
parts=[bpy.data.objects[n] for n in ['SM_Gunner_Minigun_Body','SM_Gunner_Minigun_Barrels']]
for ob in list(bpy.data.objects):
    if ob not in parts:bpy.data.objects.remove(ob,do_unlink=True)
scene=bpy.context.scene;scene.render.fps=30
data=bpy.data.armatures.new('SK_Minigun');rig=bpy.data.objects.new('Armature',data);scene.collection.objects.link(rig);bpy.context.view_layer.objects.active=rig;rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name,a,b,parent,deform in [
 ('weapon_root',(0,0,0),(0,0,10),None,True),
 ('barrel_spin',(0,-42,2.5),(0,-95,2.5),'weapon_root',True),
 ('muzzle',(0,-101.8,2.5),(0,-111.8,2.5),'weapon_root',False),
 ('grip_main',(-17,8,25),(-17,8,33),'weapon_root',False),
 ('grip_support',(0,-7,28.4),(0,-15,28.4),'weapon_root',False)]:
    bne=data.edit_bones.new(name);bne.head=a;bne.tail=b;bne.use_deform=deform
    if parent:bne.parent=data.edit_bones[parent]
bpy.ops.object.mode_set(mode='OBJECT')
for ob,group in zip(parts,['weapon_root','barrel_spin']):
    wm=ob.matrix_world.copy();ob.parent=None;ob.matrix_world=Matrix.Identity(4)
    for v in ob.data.vertices:v.co=wm@v.co
    for g in list(ob.vertex_groups):ob.vertex_groups.remove(g)
    g=ob.vertex_groups.new(name=group);g.add(list(range(len(ob.data.vertices))),1,'REPLACE');ob.hide_set(False);ob.hide_render=False
bpy.ops.object.select_all(action='DESELECT')
for ob in parts:ob.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();mesh=bpy.context.object;mesh.name='SKM_Minigun';mesh.parent=rig;mod=mesh.modifiers.new('Weapon_skin','ARMATURE');mod.object=rig
def export(name,skin=False):
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True)
    if skin:mesh.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/name),use_selection=True,object_types={'MESH','ARMATURE'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=not skin,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0,path_mode='COPY',embed_textures=True)
export('SKM_Minigun_Rigged.fbx',True)
rig.animation_data_create();report={}
for name,count,kind in [('WPN_SpinStart',16,'start'),('WPN_SpinLoop',31,'loop'),('WPN_SpinStop',16,'end')]:
    action=bpy.data.actions.new(name);action.use_fake_user=True;rig.animation_data.action=action
    for i in range(count):
        scene.frame_set(i+1);t=i/30
        angle=math.tau*8*(t*t if kind=='start' else (t-t*t if kind=='end' else t))
        for p in rig.pose.bones:
            p.rotation_mode='QUATERNION';p.matrix_basis=Matrix.Identity(4)
            if p.name=='barrel_spin':p.rotation_quaternion=Quaternion((0,1,0),angle)
            p.keyframe_insert('location',frame=i+1);p.keyframe_insert('rotation_quaternion',frame=i+1);p.keyframe_insert('scale',frame=i+1)
    for fc in action.fcurves:
        for k in fc.keyframe_points:k.interpolation='LINEAR'
    scene.frame_start=1;scene.frame_end=count;scene.frame_set(1);export(name+'.fbx')
    report[name]={'frames':count,'fps':30,'duration':(count-1)/30,'loop':kind=='loop','rpm_max':480}
rig.animation_data.action=bpy.data.actions['WPN_SpinLoop'];scene.frame_end=31;scene.frame_set(1)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':area.spaces.active.region_3d.view_distance=170;area.spaces.active.region_3d.view_location=(0,-40,3);area.spaces.active.clip_end=10000
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Minigun_Rigged_Animated.blend'))
(OUT/'weapon_animation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('WEAPON_RIG_COMPLETE',flush=True)
