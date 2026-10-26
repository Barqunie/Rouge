"""Package verified source files and UE 5.8 assets, then copy the delivery to Desktop."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

out=Path(__file__).resolve().parent
root=out.parents[2]
report=json.loads((out/'unreal_report.json').read_text())
assert report['success'] and report['orientation_matches_manny']
assert len(report['animation_checks'])==3
assert report['source_fbx_sha256']==hashlib.sha256((out/'SKM_Needle_APose.fbx').read_bytes()).hexdigest()
validation=json.loads((out/'validation_report.json').read_text())
assert all(x['passed'] for x in validation['fbx_roundtrips'].values())
previews=json.loads((out/'retarget_preview_report.json').read_text())
assert len(previews)==3
for name in ['Preview_ThreeQuarter.png','Preview_Front.png','Preview_Back.png','Preview_RigTest.png','Preview_Equipped.png','Preview_Retarget_Idle.png','Preview_Retarget_Walk.png','Preview_Retarget_Jog.png']:
    assert (out/name).stat().st_size>10000,name
assets=sorted((root/'Content/Characters/Needle').rglob('*.uasset'))
assert len(assets)>=14
files=sorted(p for p in out.iterdir() if p.suffix in {'.fbx','.glb','.blend','.py','.json','.md','.png'} and p.name!='delivery_report.json')
archive=out/'Needle_UE58_Package.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in files:z.write(p,'SourceArt/Characters/Needle/'+p.name)
    for p in assets:z.write(p,p.relative_to(root).as_posix())
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
desktop=Path.home()/'Desktop'/'Needle_UE58'
desktop.mkdir(parents=True,exist_ok=True)
for name in ['SKM_Needle_APose.fbx','SKM_Needle_Equipped_APose.fbx','SM_Needle_Dagger.fbx','T_Needle_Palette.png','README_TR.md','Preview_ThreeQuarter.png','Preview_Equipped.png','Needle_UE58_Package.zip']:
    src=out/name;dest=desktop/name
    shutil.copy2(src,dest)
    assert hashlib.sha256(src.read_bytes()).digest()==hashlib.sha256(dest.read_bytes()).digest()
delivery={'desktop':str(desktop),'zip_bytes':archive.stat().st_size,'source_files':len(files),'unreal_assets':len(assets),'copy_hashes':'match','zip_crc':'passed'}
(out/'delivery_report.json').write_text(json.dumps(delivery,indent=2),encoding='utf-8')
print(json.dumps(delivery,indent=2))
