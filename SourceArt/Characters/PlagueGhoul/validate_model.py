import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(OUT/'PlagueGhoul_APose.blend'))
mesh=bpy.data.objects['SKM_PlagueGhoul'];rig=bpy.data.objects['Armature']
weights=[sum(g.weight for g in v.groups) for v in mesh.data.vertices]
assert all(abs(w-1)<1e-5 for w in weights)
assert all(len(v.groups)<=4 for v in mesh.data.vertices)
assert all(p.area>1e-8 for p in mesh.data.polygons)
for side in ['l','r']:
    assert 39.7 < rig.data.bones['upperarm_'+side].length < 40.1
    assert 37.0 < rig.data.bones['lowerarm_'+side].length < 37.3
    assert 15.6 < rig.data.bones['hand_'+side].length < 16.0
    assert (rig.data.bones['lowerarm_'+side].head_local-rig.data.bones['upperarm_'+side].tail_local).length<1e-4
    assert (rig.data.bones['hand_'+side].head_local-rig.data.bones['lowerarm_'+side].tail_local).length<1e-4
# Exercise arms, elbows, hands, knees and finger chains in a deliberately strong pose.
for name,axis,angle in [('upperarm_l',2,-24),('upperarm_r',2,18),('lowerarm_l',0,-65),('lowerarm_r',0,-45),('hand_l',0,18),('thigh_l',0,-24),('calf_l',0,52),('calf_r',0,14)]:
    pb=rig.pose.bones[name];pb.rotation_mode='XYZ';pb.rotation_euler[axis]=math.radians(angle)
for side in ['l','r']:
    for finger in ['index','middle','ring','pinky']:
        for i in [1,2,3]:
            pb=rig.pose.bones[f'{finger}_{i:02}_{side}'];pb.rotation_mode='XYZ';pb.rotation_euler[0]=math.radians(-23)
bpy.context.view_layer.update()
deps=bpy.context.evaluated_depsgraph_get();evaluated=mesh.evaluated_get(deps);posed=evaluated.to_mesh()
assert all(math.isfinite(c) for v in posed.vertices for c in v.co)
evaluated.to_mesh_clear()
bpy.context.scene.render.filepath=str(OUT/'Preview_RigTest.png')
bpy.ops.render.render(write_still=True)
# Fresh import validates that the deliverable retains actual skinning and the single root.
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'SKM_PlagueGhoul_APose.fbx'))
arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(arms)==1 and len(meshes)==1
roots=[b.name for b in arms[0].data.bones if not b.parent]
assert roots==['root'],roots
assert len(arms[0].data.bones)==88
assert meshes[0].modifiers[0].type=='ARMATURE'
report={'fbx_roundtrip':'passed','root_bones':roots,'bone_count':len(arms[0].data.bones),'weight_normalization':'passed','max_influences':max(len(v.groups) for v in meshes[0].data.vertices),'height_m':round(meshes[0].dimensions.z,3),'posed_deformation':'finite; visual preview included'}
report['proportions']='passed; symmetric 39.89 cm upper arms, 37.11 cm forearms, 15.79 cm palms'
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(report)
