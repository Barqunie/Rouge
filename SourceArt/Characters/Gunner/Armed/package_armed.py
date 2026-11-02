from pathlib import Path
import json,shutil,zipfile,hashlib
from PIL import Image
import bpy
OUT=Path(__file__).resolve().parent;BASE=OUT.parent
TARGET=Path(r'C:\Users\Syptmpos\Desktop\Gunner_Armed_Animations');TARGET.mkdir(exist_ok=True)
report=json.loads((OUT/'animation_report.json').read_text())
validation=json.loads((OUT/'validation_report.json').read_text())
assert len(validation['fbx'])==11 and all(v=='passed' for v in validation['fbx'].values())
assert validation['frames_checked']==216
assert max(validation['fbx_pose_error_cm'].values())<.05
for gif,count in [('GUN_Walk.gif',30),('GUN_FireLoop.gif',20),('GUN_WalkFire.gif',30)]:
    im=Image.open(OUT/gif);assert im.n_frames==count,(gif,im.n_frames)
    assert sum(im.seek(i) is None and im.info['duration'] or 0 for i in range(im.n_frames))==count*50

# Keep source files focused on this character's new authored actions.
for filename,prefix in [('Gunner_Armed_Animated.blend','GUN_'),('Minigun_Rigged_Animated.blend','WPN_')]:
    bpy.ops.wm.open_mainfile(filepath=str(OUT/filename))
    for action in list(bpy.data.actions):
        if not action.name.startswith(prefix):bpy.data.actions.remove(action)
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/filename))
    shutil.copy2(OUT/filename,TARGET/filename)

for name in ['SKM_Gunner_Armed_APose.fbx']+[n+'.fbx' for n in report['clips']]+['Preview_Hold.png','Preview_Walk.png','Preview_Fire.png','Preview_WalkFire.png','GUN_Walk.gif','GUN_FireLoop.gif','GUN_WalkFire.gif','animation_report.json','validation_report.json']:
    shutil.copy2(OUT/name,TARGET/name)
weapon=TARGET/'Separate_Minigun';weapon.mkdir(exist_ok=True)
for name in ['SKM_Minigun_Rigged.fbx','WPN_SpinStart.fbx','WPN_SpinLoop.fbx','WPN_SpinStop.fbx','weapon_animation_report.json']:
    shutil.copy2(OUT/name,weapon/name)
shutil.copy2(BASE/'SM_Gunner_Minigun.fbx',weapon/'SM_Gunner_Minigun_Static.fbx')
(TARGET/'Textures').mkdir(exist_ok=True)
for p in (BASE/'Textures').glob('*.png'):shutil.copy2(p,TARGET/'Textures'/p.name)

notes=f'''GUNNER — BEL HİZASINDA MINIGUN TUTUŞU VE SİLAHLI ANİMASYONLAR

ANA MODEL
SKM_Gunner_Armed_APose.fbx: Karakter ve eldeki minigun aynı skeletal mesh'tedir.
98 kemik, {report['triangles']:,} üçgen, tek materyal. Önceki 94 kemikli karaktere 4 silah
kemiği eklenmiştir. A pozu, temel insan iskeletinin referans dönüşümlerini korur.
Ana GUN_ animasyonları bu 98 kemikli iskelete aittir.

KLİPLER — 30 FPS, IN-PLACE
GUN_HoldIdle  : 2,0 sn — iki elle silahlı bekleme, döngü.
GUN_Walk      : 1,5 sn — silahı tutarak yürüme, döngü.
GUN_FireStart : 0,5 sn — ateşe hazırlanma, namlu hızlanması, tek sefer.
GUN_FireLoop  : 1,0 sn — sürekli tarama, küçük geri tepme, döngü.
GUN_WalkFire  : 1,5 sn — yürürken tarama, döngü.
GUN_FireEnd   : 0,5 sn — ateşi bırakma, namlu yavaşlaması, tek sefer.

Karakterin ileri yer değiştirmesi oyun hareket sisteminden verilmelidir;
bu kliplerde root yatayda sabittir. Namlu dönüşü karakter kliplerine dahildir.
Bekleme -> FireStart -> FireLoop -> FireEnd -> bekleme sınır pozları eşleşir.
Yürüme ile yürürken taramanın ayak fazları ve süreleri aynıdır.

TUTUŞ
Sağ el yükseltilmiş yan arka tetik tutamağını, sol el üst taşıma kolunu kavrar.
İki el de bel hizasındadır; silah gövdesi kalça/üst bacak önünde taşınır.
Bekleme/yürümede namlu 18 derece aşağıdadır. Tararken silah belden yatay nişan alır.
Tüm karelerde iki el tutamaklara sabitlenerek pişirilmiştir.
weapon_mount, hand_r altındadır. weapon_barrels dönen parçayı yönetir.
weapon_muzzle namlu ucundadır. weapon_grip_l sol el bileğinin hedef kemiğidir.
Silah kemiği dönüşleri animasyonlarda hazırdır; ayrıca silah eklemek gerekmez.

AYRI SİLAH SEÇENEĞİ
Separate_Minigun içinde bağımsız 5 kemikli minigun ve SpinStart/SpinLoop/
SpinStop klipleri vardır. Dönüş hızı 480 RPM'dir. Statik FBX de korunmuştur.
Bu bağımsız silah klipleri kendi 5 kemikli silah iskeletine aittir.
Birleşik modelin üstüne ikinci bir silah eklemeyin; bu klasör alternatif kullanımdır.

KAYNAK VE ÖNİZLEME
Gunner_Armed_Animated.blend: Altı karakter hareketi ve takılı minigun.
Minigun_Rigged_Animated.blend: Bağımsız minigun ve üç dönüş klibi.
GIF'ler aynı gerçek modelin animasyon renderlarıdır (20 fps önizleme).
FBX'ler 30 fps'dir. Ortak doku atlası Textures klasöründedir ve FBX'e gömülüdür.

DOĞRULAMA
216 karede kol uzunlukları, tutuş hizası ve root hareketi kontrol edildi.
Döngü uçları ve ateş geçişleri karşılaştırıldı. 11 FBX yeniden açıldı;
iskelet, klip süresi ve örnek pozların dışa aktarım doğruluğu kontrol edildi.
Unreal'a aktarılmadı; UE içinde retarget testi yapılmadı.
'''
(TARGET/'OKU.txt').write_text(notes,encoding='utf-8-sig');(OUT/'README.md').write_text(notes,encoding='utf-8')
for name in ['SKM_Gunner_Armed_APose.fbx','GUN_Walk.fbx','GUN_WalkFire.fbx']:
    assert hashlib.sha256((OUT/name).read_bytes()).digest()==hashlib.sha256((TARGET/name).read_bytes()).digest()
archive=TARGET.with_suffix('.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in TARGET.rglob('*'):
        if p.is_file():z.write(p,str(Path(TARGET.name)/p.relative_to(TARGET)))
print('ARMED_PACKAGE_READY',str(TARGET),str(archive),flush=True)
