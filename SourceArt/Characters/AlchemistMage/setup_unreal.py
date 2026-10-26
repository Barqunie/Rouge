"""Run using Unreal 5.8 Python. Creates only /Game/Characters/AlchemistMage assets.
An independent skeleton is intentional: proportions differ from Manny.
"""
import unreal
import json
import traceback
import hashlib
import math
from pathlib import Path

OUT=Path(__file__).resolve().parent
DEST='/Game/Characters/AlchemistMage'
report={'revision':'2026-10-26 AlchemistMage first release','success':False}
(OUT/'unreal_report.json').write_text(json.dumps(report),encoding='utf-8')
tools=unreal.AssetToolsHelpers.get_asset_tools()
model_stats=json.loads((OUT/'model_report.json').read_text(encoding='utf-8'))

def get_or_create(name, cls, factory):
    existing=unreal.load_asset(DEST+'/'+name)
    return existing or tools.create_asset(name,DEST,cls,factory())

def main():
    unreal.EditorAssetLibrary.make_directory(DEST)
    path=DEST+'/SKM_AlchemistMage'
    model=unreal.load_asset(path)
    source_hash=hashlib.sha256((OUT/'SKM_AlchemistMage_APose.fbx').read_bytes()).hexdigest()
    previous_hash=unreal.EditorAssetLibrary.get_metadata_tag(model,'AlchemistMage.SourceSHA256') if model else ''
    previous_axes=unreal.EditorAssetLibrary.get_metadata_tag(model,'AlchemistMage.ImportAxes') if model else ''
    if model is None or model.get_editor_property('skeleton') is None or previous_hash!=source_hash or previous_axes!='Manny_YForward':
        options=unreal.FbxImportUI()
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_as_skeletal',True)
        options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property('automated_import_should_detect_type',False)
        options.set_editor_property('import_animations',False)
        options.set_editor_property('import_materials',False)
        options.set_editor_property('import_textures',False)
        options.set_editor_property('create_physics_asset',True)
        if model and model.get_editor_property('skeleton'):
            options.set_editor_property('skeleton',model.get_editor_property('skeleton'))
        data=options.get_editor_property('skeletal_mesh_import_data')
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        data.set_editor_property('import_uniform_scale',1.0)
        data.set_editor_property('convert_scene',True)
        data.set_editor_property('convert_scene_unit',True)
        # This project's Manny mesh faces +Y (its Character component supplies the yaw).
        # Keep the same mesh-space orientation so root motion and retarget chains agree.
        data.set_editor_property('force_front_x_axis',False)
        data.set_editor_property('use_t0_as_ref_pose',True)
        data.set_editor_property('update_skeleton_reference_pose',True)
        task=unreal.AssetImportTask()
        for key,value in {'filename':str(OUT/'SKM_AlchemistMage_APose.fbx'),'destination_path':DEST,'destination_name':'SKM_AlchemistMage','automated':True,'replace_existing':True,'replace_existing_settings':True,'save':True,'options':options,'factory':unreal.FbxFactory()}.items():task.set_editor_property(key,value)
        tools.import_asset_tasks([task])
        report['imported_assets']=list(task.imported_object_paths)
        model=unreal.load_asset(path)
        unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    assert isinstance(model,unreal.SkeletalMesh),'FBX did not produce a SkeletalMesh'
    report['skeletal_mesh']=model.get_path_name()
    report['bounds']=str(model.get_bounds())
    report['skeleton']=model.get_editor_property('skeleton').get_path_name()
    report['source_fbx_sha256']=source_hash
    unreal.EditorAssetLibrary.set_metadata_tag(model,'AlchemistMage.SourceSHA256',source_hash)
    unreal.EditorAssetLibrary.set_metadata_tag(model,'AlchemistMage.ImportAxes','Manny_YForward')
    # Import palette explicitly as well, avoiding dependence on FBX embedded-media handling.
    texture=unreal.load_asset(DEST+'/T_AlchemistMage_Palette')
    if not texture:
        task=unreal.AssetImportTask()
        for k,v in {'filename':str(OUT/'T_AlchemistMage_Palette.png'),'destination_path':DEST,'destination_name':'T_AlchemistMage_Palette','automated':True,'save':True}.items():task.set_editor_property(k,v)
        tools.import_asset_tasks([task]);texture=unreal.load_asset(DEST+'/T_AlchemistMage_Palette')
    texture.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
    materials=[]
    for glow in [False,True]:
        name='M_AM_Toxic' if glow else 'M_AM_Body'
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
    for name,skmesh in [('IK_Manny_ForAlchemistMage',source),('IK_AlchemistMage',model)]:
        rig=get_or_create(name,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory)
        ctrl=unreal.IKRigController.get_controller(rig)
        assert ctrl.set_skeletal_mesh(skmesh)
        for old in ctrl.get_retarget_chains(): ctrl.remove_retarget_chain(old.chain_name)
        ctrl.set_retarget_root('pelvis')
        if hasattr(ctrl,'set_root_motion_bone'): ctrl.set_root_motion_bone('root')
        for cname,start,end in chains:ctrl.add_retarget_chain(cname,start,end,'')
        report[name+'_auto_fbik']=ctrl.apply_auto_fbik() if hasattr(ctrl,'apply_auto_fbik') else False
        unreal.EditorAssetLibrary.save_loaded_asset(rig);rigs.append(rig)
    retarget=get_or_create('RTG_Manny_to_AlchemistMage',unreal.IKRetargeter,unreal.IKRetargetFactory)
    ctrl=unreal.IKRetargeterController.get_controller(retarget)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,rigs[0]);ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,rigs[1])
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE,source);ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET,model)
    ctrl.remove_all_ops();ctrl.add_default_ops();ctrl.auto_map_chains(unreal.AutoMapChainType.EXACT,True)
    target_mode=unreal.RetargetSourceOrTarget.TARGET
    ctrl.reset_retarget_pose(ctrl.get_current_retarget_pose_name(target_mode),[],target_mode)
    # Single-bone head chains cannot infer a useful direction from the jaw child.
    # Align the limbs only; preserve the authored head/neck rest pose.
    ctrl.auto_align_bones(['upperarm_l','lowerarm_l','upperarm_r','lowerarm_r','thigh_l','calf_l','thigh_r','calf_r'],unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN,target_mode)
    report['chain_mapping']={name:str(ctrl.get_source_chain(name)) for name,_,_ in chains}
    unreal.EditorAssetLibrary.save_loaded_asset(retarget)
    report['retargeter']=retarget.get_path_name()
    source_animations=[unreal.load_asset('/Game/Characters/Mannequins/Anims/Unarmed/'+p) for p in ['MM_Idle','Walk/MF_Unarmed_Walk_Fwd','Jog/MF_Unarmed_Jog_Fwd']]
    assert all(source_animations),'Idle, walk and jog validation animations must exist.'
    for label,skmesh,anim in [('source_manny',source,source_animations[0]),('target_mage',model,unreal.load_asset(DEST+'/Validation/AM_MM_Idle_Verified'))]:
        if anim:
            ev=unreal.AnimPoseEvaluationOptions();ev.set_editor_property('optional_skeletal_mesh',skmesh)
            ap=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,0,ev)
            report[label+'_reference_joints']={n:[getattr(unreal.AnimPoseExtensions.get_ref_bone_pose(ap,n,unreal.AnimPoseSpaces.WORLD).translation,c) for c in ['x','y','z']] for n in ['upperarm_l','upperarm_r','foot_l','ball_l','head']}
    if source_animations:
        inputs=unreal.IKRetargetBatchOperationInputs()
        for k,v in {'assets_to_retarget':[unreal.EditorAssetLibrary.find_asset_data(anim.get_path_name()) for anim in source_animations],'source_mesh':source,'target_mesh':model,'ik_retarget_asset':retarget,'prefix':'AM_','suffix':'_Verified','target_path':DEST+'/Validation','include_referenced_assets':False,'overwrite_existing_files':True}.items(): inputs.set_editor_property(k,v)
        result=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        report['retargeted_animations']=[str(a.package_name) for a in result]
        assert len(result)==3,'Idle, walk and jog must all retarget successfully'
        qa=OUT.parents[2]/'Saved'/'CodexModeling'/'mage_20261026';qa.mkdir(parents=True,exist_ok=True)
        report['animation_checks']={}
        def encode_transform(t):
            return {'translation':[t.translation.x,t.translation.y,t.translation.z],'rotation':[t.rotation.w,t.rotation.x,t.rotation.y,t.rotation.z],'scale':[t.scale3d.x,t.scale3d.y,t.scale3d.z]}
        for item in result:
            anim=item.get_asset();anim.set_preview_skeletal_mesh(model)
            unreal.EditorAssetLibrary.save_loaded_asset(anim)
            evaluation=unreal.AnimPoseEvaluationOptions()
            evaluation.set_editor_property('optional_skeletal_mesh',model)
            lengths=[];samples=[];reference={}
            for frame in range(round(anim.get_play_length()*30)+1):
                fraction=frame/max(1,round(anim.get_play_length()*30))
                pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,anim.get_play_length()*fraction,evaluation)
                names=unreal.AnimPoseExtensions.get_bone_names(pose)
                if not reference:
                    reference={str(n):encode_transform(unreal.AnimPoseExtensions.get_ref_bone_pose(pose,n,unreal.AnimPoseSpaces.WORLD)) for n in names}
                    src=report['source_manny_reference_joints']
                    target={n:t['translation'] for n,t in reference.items()}
                    for end,start in [('ball_l','foot_l'),('upperarm_l','upperarm_r')]:
                        a=[src[end][i]-src[start][i] for i in [0,1]]
                        b=[target[end][i]-target[start][i] for i in [0,1]]
                        dot=sum(x*y for x,y in zip(a,b))/math.sqrt(sum(x*x for x in a)*sum(x*x for x in b))
                        assert dot>.97,('Source/target mesh orientation mismatch',end,dot)
                    report['orientation_matches_manny']=True
                samples.append({'fraction':fraction,'bones':{str(n):encode_transform(unreal.AnimPoseExtensions.get_bone_pose(pose,n,unreal.AnimPoseSpaces.WORLD)) for n in names}})
                assert all(math.isfinite(c) for t in samples[-1]['bones'].values() for field in ['translation','rotation','scale'] for c in t[field])
                for side in ['l','r']:
                    points=[unreal.AnimPoseExtensions.get_bone_pose(pose,b+'_'+side,unreal.AnimPoseSpaces.WORLD).translation for b in ['upperarm','lowerarm','hand']]
                    assert all(math.isfinite(c) for p in points for c in [p.x,p.y,p.z])
                    distances=[math.sqrt(sum((getattr(points[i],c)-getattr(points[i+1],c))**2 for c in ['x','y','z'])) for i in [0,1]]
                    assert abs(distances[0]-model_stats['upperarm_cm'])<.5 and abs(distances[1]-model_stats['forearm_cm'])<.5,distances
                    lengths.append([round(x,3) for x in distances])
                    for a,b in [('thigh','calf'),('calf','foot')]:
                        pa=samples[-1]['bones'][a+'_'+side]['translation'];pb=samples[-1]['bones'][b+'_'+side]['translation']
                        ra=reference[a+'_'+side]['translation'];rb=reference[b+'_'+side]['translation']
                        posed_len=math.sqrt(sum((x-y)**2 for x,y in zip(pa,pb)))
                        ref_len=math.sqrt(sum((x-y)**2 for x,y in zip(ra,rb)))
                        assert abs(posed_len-ref_len)<.5,(a,b,posed_len,ref_len)
            report['animation_checks'][anim.get_name()]={'sampled_times':len(samples),'both_arms_checked':True,'upper_forearm_cm':lengths}
            (qa/('Retarget_'+anim.get_name()+'.json')).write_text(json.dumps({'animation':anim.get_path_name(),'reference':reference,'samples':samples,'duration':anim.get_play_length(),'fps':30}),encoding='utf-8')
    unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    report['success']=True

try:
    main()
except Exception:
    report['success']=False;report['error']=traceback.format_exc();unreal.log_error(report['error'])
finally:
    (OUT/'unreal_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('MAGE_REPORT '+json.dumps(report))
