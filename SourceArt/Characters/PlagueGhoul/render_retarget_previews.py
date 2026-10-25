"""Render bone poses sampled from actual Unreal retargeted animations.
setup_unreal.py writes the samples in Saved/CodexModeling/revision_20261021.
Coordinate conversion is fitted and checked against the imported reference pose.
"""
import bpy
import json
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

OUT=Path(__file__).resolve().parent
QA=OUT.parents[2]/'Saved'/'CodexModeling'/'revision_20261021'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'PlagueGhoul_APose.blend'))
rig=bpy.data.objects['Armature']
scene=bpy.context.scene
scene.render.resolution_x=1100;scene.render.resolution_y=1100
scene.cycles.samples=32

def transform(t):
    return Matrix.LocRotScale(Vector(t['translation']),Quaternion(t['rotation']),Vector(t['scale']))

report={}
for suffix,label in [('MM_Idle','Idle'),('MF_Unarmed_Walk_Fwd','Walk'),('MF_Unarmed_Jog_Fwd','Jog')]:
    rig.location=(0,0,0)
    data=json.loads((QA/f'Retarget_PG_{suffix}_Verified.json').read_text())
    names=[b.name for b in rig.data.bones if b.name in data['reference']]
    ue=np.array([data['reference'][name]['translation']+[1.] for name in names])
    bl=np.array([list(rig.data.bones[name].head_local) for name in names])
    coeff=np.linalg.lstsq(ue,bl,rcond=None)[0]
    fit_error=float(np.max(np.linalg.norm(ue@coeff-bl,axis=1)))
    assert fit_error<.01,('Reference-pose coordinate mapping failed',fit_error)
    convert=Matrix.Identity(4)
    for row in range(3):
        for col in range(4):convert[row][col]=float(coeff[col,row])
    inverse=convert.inverted()
    sample=data['samples'][1]
    desired={}
    for name in names:
        posed=transform(sample['bones'][name]);rest=transform(data['reference'][name])
        desired[name]=convert@posed@rest.inverted()@inverse@rig.data.bones[name].matrix_local
    for name in names:
        pb=rig.pose.bones[name];rest=pb.bone.matrix_local
        if pb.parent:
            pb.matrix_basis=rest.inverted()@pb.parent.bone.matrix_local@desired[pb.parent.name].inverted()@desired[name]
        else:pb.matrix_basis=rest.inverted()@desired[name]
    bpy.context.view_layer.update()
    error=max((rig.pose.bones[name].matrix.translation-desired[name].translation).length for name in names)
    assert error<.02,('Pose reconstruction error',error)
    # Present root-motion clips in place so the character stays in the studio frame.
    root_motion=convert@Vector(sample['bones']['root']['translation'])-convert@Vector(data['reference']['root']['translation'])
    rig.location=-root_motion
    bpy.context.view_layer.update()
    report[label]={'reference_mapping_error_cm':fit_error,'pose_reconstruction_error_cm':error,'unreal_animation':data['animation'],'time_fraction':sample['fraction']}
    scene.render.filepath=str(OUT/f'Preview_Retarget_{label}.png')
    bpy.ops.render.render(write_still=True)
(OUT/'retarget_preview_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('RETARGET_PREVIEWS_VERIFIED',report,flush=True)
