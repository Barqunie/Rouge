# Shared palette atlas: one material slot for the character and one for each prop.
texdir=OUT/'Textures';texdir.mkdir(exist_ok=True)
names=list(MATDEF);images={k:Image.new('RGB',(512,256),(0,0,0)) for k in ['BaseColor','Roughness','Metallic','Emissive']}
for i,name in enumerate(names):
    hexc,rough,metal=MATDEF[name];rgb=tuple(int(hexc[k:k+2],16) for k in [1,3,5]);x=i%8*64;y=i//8*64
    colors={'BaseColor':rgb,'Roughness':(round(rough*255),)*3,'Metallic':(round(metal*255),)*3,'Emissive':rgb if name=='Toxic' else (0,0,0)}
    for channel,img in images.items():img.paste(colors[channel],(x,y,x+64,y+64))
atlas=bpy.data.materials.new('M_Gunner_Palette');atlas.use_nodes=True
nodes=atlas.node_tree.nodes;p=nodes.get('Principled BSDF');p.inputs['Specular IOR Level'].default_value=.32;p.inputs['Emission Strength'].default_value=.3
for channel,img in images.items():
    path=texdir/f'Gunner_{channel}.png';img.save(path)
    image=bpy.data.images.load(str(path),check_existing=False)
    if channel in ['Roughness','Metallic']:image.colorspace_settings.name='Non-Color'
    tex=nodes.new('ShaderNodeTexImage');tex.image=image;tex.interpolation='Closest';tex.label=channel
    socket={'BaseColor':'Base Color','Roughness':'Roughness','Metallic':'Metallic','Emissive':'Emission Color'}[channel]
    atlas.node_tree.links.new(tex.outputs['Color'],p.inputs[socket]);image.pack()

def joinparts(parts,name,skin=False):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=name
    bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.triangulate(bm,faces=list(bm.faces),quad_method='BEAUTY',ngon_method='BEAUTY')
    bad=[f for f in bm.faces if f.calc_area()<1e-9]
    if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free();ob.data.update()
    uv=ob.data.uv_layers.active
    for poly in ob.data.polygons:
        material=ob.data.materials[poly.material_index];key=next(n for n,m in MATS.items() if m==material);index=names.index(key)
        cx=(index%8+.5)/8;cy=1-(index//8+.5)/4
        for j,li in enumerate(poly.loop_indices):
            angle=j*math.tau/len(poly.loop_indices);uv.data[li].uv=(cx+math.cos(angle)*.025,cy+math.sin(angle)*.050)
        poly.material_index=0;poly.use_smooth=False
    ob.data.materials.clear();ob.data.materials.append(atlas)
    if skin:
        mod=ob.modifiers.new('Gunner_skin','ARMATURE');mod.object=rig;ob.parent=rig
        assert all(v.groups and len(v.groups)<=4 and abs(sum(g.weight for g in v.groups)-1)<1e-4 for v in ob.data.vertices)
    else:
        for g in list(ob.vertex_groups):ob.vertex_groups.remove(g)
    return ob

character=joinparts(CHAR_PARTS,'SKM_Gunner',True)
receiver=joinparts(RECEIVER_PARTS,'SM_Gunner_Minigun_Body')
barrels=joinparts(BARREL_PARTS,'SM_Gunner_Minigun_Barrels')
# The barrel cluster has a useful pivot on its rotation axis.
pivot=Vector((0,-.42,.025))*S
for v in barrels.data.vertices:v.co-=pivot
barrels.location=pivot
def export_fbx(path,objects,skin=False):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','ARMATURE'} if skin else {'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=False,path_mode='COPY',embed_textures=True)
export_fbx(OUT/'SKM_Gunner_APose.fbx',[rig,character],True)
# Export the combined gun as one mesh, plus optional separate pieces for barrel rotation.
copies=[]
for ob in [receiver,barrels]:
    dup=ob.copy();dup.data=ob.data.copy();scene.collection.objects.link(dup);copies.append(dup)
bpy.ops.object.select_all(action='DESELECT')
for ob in copies:ob.select_set(True)
bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();whole=bpy.context.object;whole.name='SM_Gunner_Minigun'
export_fbx(OUT/'SM_Gunner_Minigun.fbx',[whole]);guntris=len(whole.data.polygons);bpy.data.objects.remove(whole,do_unlink=True)
partsdir=OUT/'Minigun_Parts';partsdir.mkdir(exist_ok=True)
export_fbx(partsdir/'SM_Gunner_Minigun_Body.fbx',[receiver])
saved=barrels.location.copy();barrels.location=(0,0,0);export_fbx(partsdir/'SM_Gunner_Minigun_Barrels.fbx',[barrels]);barrels.location=saved
stats={'character_vertices':len(character.data.vertices),'character_triangles':len(character.data.polygons),'character_height_cm':round(character.dimensions.z,2),'bones':len(rig.data.bones),'root':'root','pose':'A pose','material_slots':len(character.material_slots),'max_influences':max(len(v.groups) for v in character.data.vertices),'unweighted_vertices':sum(not v.groups for v in character.data.vertices),'minigun_triangles':guntris,'minigun_barrels':6,'barrel_part_offset_blender_cm':list(pivot),'texture_atlas':'512x256; base color, roughness, metallic, emissive','core_skeleton':'Manny-style names, same core rest transforms as AlchemistMage; own skeleton for IK retargeting','delivery':'FBX only; no Unreal import'}
(OUT/'model_report.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
print('GUNNER_EXPORTED',json.dumps(stats),flush=True)

# Studio cameras and lighting. All previews are renders of the actual exported geometry.
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.18,.20,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
scene.view_settings.view_transform='AgX'
floor=bpy.data.materials.new('Studio_floor');floor.use_nodes=True;floor.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.045,.05,.055,1);floor.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.9
bpy.ops.mesh.primitive_plane_add(size=20000,location=(0,0,-.1));ground=bpy.context.object;ground.name='STUDIO_Ground';ground.data.materials.append(floor)
def point_at(ob,target):ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
for name,loc,power,size,color in [('Key',(-230,-320,340),2350000,180,(1,.89,.76)),('Fill',(230,-230,260),1400000,220,(.80,.88,1)),('Rim',(140,220,320),2500000,160,(.91,1,.73))]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    ob=bpy.data.objects.new(name,data);scene.collection.objects.link(ob);ob.location=loc;point_at(ob,(0,0,115))
data=bpy.data.cameras.new('Preview_camera');camera=bpy.data.objects.new('Preview_camera',data);scene.collection.objects.link(camera);scene.camera=camera;data.type='ORTHO';data.clip_end=10000
camera.location=(240,-730,204);point_at(camera,(0,0,96));camera.data.ortho_scale=211
receiver.hide_render=True;barrels.hide_render=True;receiver.hide_set(True);barrels.hide_set(True)
bpy.ops.object.select_all(action='DESELECT');character.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':area.spaces.active.region_3d.view_distance=280;area.spaces.active.region_3d.view_location=(0,0,99);area.spaces.active.clip_end=10000
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Gunner_APose.blend'))
for name,pos,target,scale,res in [
 ('ThreeQuarter',(240,-730,204),(0,0,96),211,(1200,1450)),
 ('Face',(40,-270,178),(0,0,174),43,(1200,1200)),
 ('Front',(0,-750,99),(0,0,99),211,(1200,1450)),
 ('Back',(240,730,202),(0,0,96),211,(1200,1450))]:
    camera.location=pos;point_at(camera,target);camera.data.ortho_scale=scale;scene.render.resolution_x=res[0];scene.render.resolution_y=res[1];scene.render.filepath=str(OUT/f'Preview_{name}.png');bpy.ops.render.render(write_still=True)
    print('RENDER_READY',name,flush=True)
character.hide_render=True;receiver.hide_render=False;barrels.hide_render=False;receiver.hide_set(False);barrels.hide_set(False);ground.location.z=-17
camera.location=(175,-235,135);point_at(camera,(0,-42,2));camera.data.ortho_scale=130;scene.render.resolution_x=1500;scene.render.resolution_y=1050
scene.render.filepath=str(OUT/'Preview_Minigun.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Minigun_Source.blend'))
print('GUNNER_BUILD_COMPLETE',flush=True)
