"""Import each baked FBX on the mage skeleton, then compare every frame to Blender."""
import unreal,json,math,traceback
from pathlib import Path
OUT=Path(__file__).resolve().parent;DEST='/Game/Characters/AlchemistMage'
QA=OUT.parents[2]/'Saved/CodexModeling/mage_20261026'
report={'success':False,'clips':{}}
def main():
    model=unreal.load_asset(DEST+'/SKM_AlchemistMage');skeleton=model.get_editor_property('skeleton')
    expected=json.loads((QA/'animation_expected.json').read_text())
    animinfo=json.loads((OUT/'animation_report.json').read_text())['clips']
    asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
    for name,info in animinfo.items():
        options=unreal.FbxImportUI()
        for k,v in {'import_mesh':False,'import_as_skeletal':True,'import_animations':True,'mesh_type_to_import':unreal.FBXImportType.FBXIT_ANIMATION,'automated_import_should_detect_type':False,'skeleton':skeleton,'import_materials':False,'import_textures':False}.items():options.set_editor_property(k,v)
        data=options.get_editor_property('anim_sequence_import_data')
        for k,v in {'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'import_uniform_scale':1.,'use_default_sample_rate':True,'import_bone_tracks':True,'animation_length':unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME}.items():data.set_editor_property(k,v)
        task=unreal.AssetImportTask()
        for k,v in {'filename':str(OUT/(name+'.fbx')),'destination_path':DEST+'/Animations','destination_name':name,'automated':True,'replace_existing':True,'replace_existing_settings':True,'save':True,'options':options,'factory':unreal.FbxFactory()}.items():task.set_editor_property(k,v)
        asset_tools.import_asset_tasks([task])
        anim=unreal.load_asset(DEST+'/Animations/'+name)
        if not anim:
            imported=[unreal.load_asset(p) for p in task.imported_object_paths]
            anim=next((p for p in imported if isinstance(p,unreal.AnimSequence)),None)
        assert isinstance(anim,unreal.AnimSequence),(name,list(task.imported_object_paths))
        anim.set_preview_skeletal_mesh(model)
        anim.set_editor_property('enable_root_motion',False)
        anim.set_editor_property('force_root_lock',True)
        assert anim.get_editor_property('skeleton')==skeleton
        assert abs(anim.get_play_length()-info['duration_seconds'])<.001,(name,anim.get_play_length())
        unreal.EditorAssetLibrary.set_metadata_tag(anim,'AlchemistMage.Motion','InPlace')
        if name=='AM_Cast':
            unreal.EditorAssetLibrary.set_metadata_tag(anim,'AlchemistMage.ReleaseTimeSeconds','0.60')
        evaluation=unreal.AnimPoseEvaluationOptions();evaluation.set_editor_property('optional_skeletal_mesh',model)
        maxerror=0.;maxbone='';samples=[];reference={}
        # FBX axes from this project's bind-pose import: Blender(x,y,z) -> UE(x,-y,z).
        # Determine signs from the actual reference data rather than assuming this.
        mapping=None
        for sample in expected['clips'][name]['samples']:
            pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,sample['time'],evaluation)
            if mapping is None:
                names=list(sample['heads_blender'])
                reference={n:unreal.AnimPoseExtensions.get_ref_bone_pose(pose,n,unreal.AnimPoseSpaces.WORLD) for n in names}
                import itertools
                candidates=[]
                for perm in itertools.permutations(range(3)):
                    for signs in itertools.product([-1,1],repeat=3):
                        err=0
                        for n in ['head','upperarm_l','upperarm_r','foot_l','ball_l']:
                            p=expected['reference_blender'][n];r=reference[n].translation
                            err+=sum((getattr(r,'xyz'[i])-p[perm[i]]*signs[i])**2 for i in range(3))
                        candidates.append((err,perm,signs))
                err,perm,signs=min(candidates)
                assert err<.01,('Reference mismatch',err)
                mapping=(perm,signs)
            perm,signs=mapping;heads={}
            for n,p in sample['heads_blender'].items():
                value=unreal.AnimPoseExtensions.get_bone_pose(pose,n,unreal.AnimPoseSpaces.WORLD).translation
                vec=[value.x,value.y,value.z];heads[n]=vec
                err=math.sqrt(sum((vec[i]-p[perm[i]]*signs[i])**2 for i in range(3)))
                assert all(math.isfinite(v) for v in vec)
                if err>maxerror:maxerror=err;maxbone=n
            samples.append({'frame':sample['frame'],'heads':heads})
        assert maxerror<.35,(name,maxerror,maxbone)
        report['clips'][name]={'asset':anim.get_path_name(),'duration':anim.get_play_length(),'frames_checked':len(samples),'max_bone_head_error_cm':maxerror,'max_error_bone':maxbone,'skeleton_matches':True,'in_place':True}
        unreal.EditorAssetLibrary.save_loaded_asset(anim)
        (QA/(name+'_ImportedSamples.json')).write_text(json.dumps(samples),encoding='utf-8')
    unreal.EditorAssetLibrary.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    report['success']=True
try:main()
except Exception:
    report['error']=traceback.format_exc();unreal.log_error(report['error'])
finally:
    (OUT/'unreal_animation_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('MAGE_ANIMATION_REPORT '+json.dumps(report))
