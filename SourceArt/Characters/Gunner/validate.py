"""Check actual FBX files, skin, core rest pose and sampled deformation."""
import bpy,json,math
from pathlib import Path
OUT=Path(__file__).resolve().parent;BASE=OUT.parent/'AlchemistMage'
stats=json.loads((OUT/'model_report.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Gunner_APose.blend'))
rig=bpy.data.objects['Armature'];model=bpy.data.objects['SKM_Gunner'];scene=bpy.context.scene
assert len(rig.data.bones)==stats['bones']==94
assert [b.name for b in rig.data.bones if not b.parent]==['root']
assert all(0<len(v.groups)<=4 and abs(sum(g.weight for g in v.groups)-1)<1e-4 for v in model.data.vertices)
assert all(p.area>1e-9 and len(p.vertices)==3 for p in model.data.polygons)
assert len(model.material_slots)==1 and len(model.data.uv_layers)==1
assert all(g.name in rig.data.bones for g in model.vertex_groups)
with bpy.data.libraries.load(str(BASE/'AlchemistMage_APose.blend'),link=False) as (src,dst):dst.armatures=['SK_AlchemistMage']
old=dst.armatures[0]
core=[b for b in rig.data.bones if not b.name.startswith('hair_')]
error=max(abs(b.matrix_local[i][j]-old.bones[b.name].matrix_local[i][j]) for b in core for i in range(4) for j in range(4))
assert error<1e-5
report={'core_rest_transform_error_cm':error,'core_bones_compared':len(core),'bones':94,'single_root':'root','weights':'normalized, no unweighted vertices, maximum four influences','character_triangles':len(model.data.polygons),'single_palette_material':True,'pose_checks':{}}
with bpy.data.libraries.load(str(BASE/'AlchemistMage_Animated.blend'),link=False) as (src,dst):dst.actions=[n for n in ['AM_Idle','AM_Walk','AM_Run','AM_Cast'] if n in src.actions]
assert len(dst.actions)==4
rig.animation_data_create();scene.cycles.samples=24
for action in dst.actions:
    rig.animation_data.action=action;count=int(action.frame_range.y)
    for frame in [1,count//4,count//2,3*count//4,count]:
        scene.frame_set(max(1,frame));ev=model.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=ev.to_mesh()
        assert all(math.isfinite(c) for v in mesh.vertices for c in v.co)
        assert max(v.co.length for v in mesh.vertices)<240
        ev.to_mesh_clear()
    report['pose_checks'][action.name]='5 samples: finite positions, no exploded vertices'
    if action.name in ['AM_Run','AM_Cast']:
        scene.frame_set(7 if action.name=='AM_Run' else 19);scene.render.resolution_x=900;scene.render.resolution_y=1100;scene.render.filepath=str(OUT/('QA_'+action.name+'.png'));bpy.ops.render.render(write_still=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'SKM_Gunner_APose.fbx'));bpy.context.view_layer.update()
arms=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(arms)==1 and len(meshes)==1
assert len(arms[0].data.bones)==94 and [b.name for b in arms[0].data.bones if not b.parent]==['root']
assert len(meshes[0].data.polygons)==stats['character_triangles']
assert all(v.groups for v in meshes[0].data.vertices)
assert len(meshes[0].material_slots)==1 and not bpy.data.actions
# read_factory_settings replaces the scene object; use the active imported scene for units.
height=meshes[0].dimensions.z*bpy.context.scene.unit_settings.scale_length*100
assert abs(height-stats['character_height_cm'])<.1,(height,stats['character_height_cm'])
material=meshes[0].data.materials[0];texture_nodes=[n for n in material.node_tree.nodes if n.type=='TEX_IMAGE']
assert any(n.image and n.image.has_data for n in texture_nodes)
report['character_fbx']='passed: one mesh, one armature, 94 bones, one material, A pose only'
report['height_after_fbx_cm']=round(height,3)
report['fbx_texture_image_loaded']=True

for file,expected in [('SM_Gunner_Minigun.fbx',stats['minigun_triangles']),('Minigun_Parts/SM_Gunner_Minigun_Body.fbx',None),('Minigun_Parts/SM_Gunner_Minigun_Barrels.fbx',None)]:
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(OUT/file))
    meshes=[o for o in bpy.data.objects if o.type=='MESH'];assert len(meshes)==1
    assert not any(o.type=='ARMATURE' for o in bpy.data.objects)
    assert len(meshes[0].material_slots)==1
    if expected:assert len(meshes[0].data.polygons)==expected
    assert all(p.area>1e-13 for p in meshes[0].data.polygons)
    report[file]='passed: separate static mesh, one material'
report['Unreal_import']='Not performed, FBX delivery only'
(OUT/'validation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('GUNNER_VALIDATION_COMPLETE',json.dumps(report),flush=True)
