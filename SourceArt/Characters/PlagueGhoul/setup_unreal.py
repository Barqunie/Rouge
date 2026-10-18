"""Run using Unreal 5.8 Python. Creates only /Game/Characters/PlagueGhoul assets.
An independent skeleton is intentional: proportions differ from Manny.
"""
import unreal
import json
import traceback
from pathlib import Path

OUT=Path(__file__).resolve().parent
DEST='/Game/Characters/PlagueGhoul'
report={}
tools=unreal.AssetToolsHelpers.get_asset_tools()

def get_or_create(name, cls, factory):
    existing=unreal.load_asset(DEST+'/'+name)
    return existing or tools.create_asset(name,DEST,cls,factory())

def main():
    unreal.EditorAssetLibrary.make_directory(DEST)
    path=DEST+'/SKM_PlagueGhoul'
    model=unreal.load_asset(path)
    if model is None or model.get_editor_property('skeleton') is None:
        options=unreal.FbxImportUI()
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_as_skeletal',True)
        options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property('automated_import_should_detect_type',False)
        options.set_editor_property('import_animations',False)
        options.set_editor_property('import_materials',True)
        options.set_editor_property('import_textures',True)
        options.set_editor_property('create_physics_asset',True)
        data=options.get_editor_property('skeletal_mesh_import_data')
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        data.set_editor_property('import_uniform_scale',1.0)
        data.set_editor_property('convert_scene',True)
        data.set_editor_property('convert_scene_unit',True)
        data.set_editor_property('force_front_x_axis',True)
        data.set_editor_property('use_t0_as_ref_pose',True)
        task=unreal.AssetImportTask()
        for key,value in {'filename':str(OUT/'SKM_PlagueGhoul_APose.fbx'),'destination_path':DEST,'destination_name':'SKM_PlagueGhoul','automated':True,'replace_existing':True,'replace_existing_settings':True,'save':True,'options':options,'factory':unreal.FbxFactory()}.items():task.set_editor_property(key,value)
        tools.import_asset_tasks([task])
        report['imported_assets']=list(task.imported_object_paths)
        model=unreal.load_asset(path)
        unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    assert isinstance(model,unreal.SkeletalMesh),'FBX did not produce a SkeletalMesh'
    report['skeletal_mesh']=model.get_path_name()
    report['bounds']=str(model.get_bounds())
    report['skeleton']=model.get_editor_property('skeleton').get_path_name()
    # Import palette explicitly as well, avoiding dependence on FBX embedded-media handling.
    texture=unreal.load_asset(DEST+'/T_PlagueGhoul_Palette')
    if not texture:
        task=unreal.AssetImportTask()
        for k,v in {'filename':str(OUT/'T_PlagueGhoul_Palette.png'),'destination_path':DEST,'destination_name':'T_PlagueGhoul_Palette','automated':True,'save':True}.items():task.set_editor_property(k,v)
        tools.import_asset_tasks([task]);texture=unreal.load_asset(DEST+'/T_PlagueGhoul_Palette')
    texture.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
    materials=[]
    for glow in [False,True]:
        name='M_PG_Toxic' if glow else 'M_PG_Body'
        mat=get_or_create(name,unreal.Material,unreal.MaterialFactoryNew)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
        mat.set_editor_property('two_sided',True)
        sampler=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0)
        sampler.set_editor_property('texture',texture)
        unreal.MaterialEditingLibrary.connect_material_property(sampler,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,200)
        rough.set_editor_property('r',.85)
        unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        if glow:
            mul=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionMultiply,-150,-180)
            mul.set_editor_property('const_b',.35)
            unreal.MaterialEditingLibrary.connect_material_expressions(sampler,'RGB',mul,'A')
            unreal.MaterialEditingLibrary.connect_material_property(mul,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        materials.append(mat)
    slots=model.get_editor_property('materials')
    for i,slot in enumerate(slots):
        slot.set_editor_property('material_interface',materials[1 if 'Toxic' in str(slot.material_slot_name) else 0])
        slots[i]=slot
    model.set_editor_property('materials',slots)
    report['materials']=[str(s.material_interface.get_path_name()) for s in model.get_editor_property('materials')]
    unreal.EditorAssetLibrary.save_loaded_asset(model)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    source=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    assert source,'Manny source mesh not found; change source path for a different project.'
    chains=[('Root','root','root'),('Spine','spine_01','spine_05'),('Neck','neck_01','neck_02'),('Head','head','head')]
    for side,longname in [('l','Left'),('r','Right')]:
        chains += [(longname+'Clavicle','clavicle_'+side,'clavicle_'+side),(longname+'Arm','upperarm_'+side,'hand_'+side),(longname+'Leg','thigh_'+side,'foot_'+side),(longname+'Toe','ball_'+side,'ball_'+side)]
        for f in ['thumb','index','middle','ring','pinky']: chains.append((longname+f.title(),f+'_01_'+side,f+'_03_'+side))
    rigs=[]
    for name,skmesh in [('IK_Manny_ForPlagueGhoul',source),('IK_PlagueGhoul',model)]:
        rig=get_or_create(name,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory)
        ctrl=unreal.IKRigController.get_controller(rig)
        assert ctrl.set_skeletal_mesh(skmesh)
        for old in ctrl.get_retarget_chains(): ctrl.remove_retarget_chain(old.chain_name)
        ctrl.set_retarget_root('pelvis')
        if hasattr(ctrl,'set_root_motion_bone'): ctrl.set_root_motion_bone('root')
        for cname,start,end in chains:ctrl.add_retarget_chain(cname,start,end,'')
        report[name+'_auto_fbik']=ctrl.apply_auto_fbik() if hasattr(ctrl,'apply_auto_fbik') else False
        unreal.EditorAssetLibrary.save_loaded_asset(rig);rigs.append(rig)
    retarget=get_or_create('RTG_Manny_to_PlagueGhoul',unreal.IKRetargeter,unreal.IKRetargetFactory)
    ctrl=unreal.IKRetargeterController.get_controller(retarget)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,rigs[0]);ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,rigs[1])
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE,source);ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET,model)
    ctrl.add_default_ops();ctrl.auto_map_chains(unreal.AutoMapChainType.EXACT,True)
    target_mode=unreal.RetargetSourceOrTarget.TARGET
    ctrl.reset_retarget_pose(ctrl.get_current_retarget_pose_name(target_mode),[],target_mode)
    # Single-bone head chains cannot infer a useful direction from the jaw child.
    # Align the limbs only; preserve the authored head/neck rest pose.
    ctrl.auto_align_bones(['upperarm_l','lowerarm_l','upperarm_r','lowerarm_r','thigh_l','calf_l','thigh_r','calf_r'],unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN,target_mode)
    report['chain_mapping']={name:str(ctrl.get_source_chain(name)) for name,_,_ in chains}
    unreal.EditorAssetLibrary.save_loaded_asset(retarget)
    report['retargeter']=retarget.get_path_name()
    anim=unreal.load_asset('/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle')
    if anim:
        inputs=unreal.IKRetargetBatchOperationInputs()
        for k,v in {'assets_to_retarget':[unreal.EditorAssetLibrary.find_asset_data(anim.get_path_name())],'source_mesh':source,'target_mesh':model,'ik_retarget_asset':retarget,'prefix':'PG_','suffix':'_Verified','target_path':DEST+'/Animations','include_referenced_assets':False,'overwrite_existing_files':True}.items(): inputs.set_editor_property(k,v)
        result=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        report['retargeted_animations']=[str(a.package_name) for a in result]
        assert result,'Sample retarget generated no animation'
    unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    report['success']=True

try:
    main()
except Exception:
    report['success']=False;report['error']=traceback.format_exc();unreal.log_error(report['error'])
finally:
    (OUT/'unreal_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('PLAGUE_GHOUL_REPORT '+json.dumps(report))
