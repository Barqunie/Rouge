from pathlib import Path
import json,shutil,hashlib,zipfile
from PIL import Image,ImageDraw,ImageFont
OUT=Path(__file__).resolve().parent
TARGET=Path(r'C:\Users\Syptmpos\Desktop\Gunner_LowPoly_FBX')
stats=json.loads((OUT/'model_report.json').read_text())
validation=json.loads((OUT/'validation_report.json').read_text())
assert stats['character_triangles']==validation['character_triangles']
assert validation['character_fbx'].startswith('passed')
TARGET.mkdir(exist_ok=True)
files=['SKM_Gunner_APose.fbx','SM_Gunner_Minigun.fbx','Gunner_APose.blend','Minigun_Source.blend','Preview_ThreeQuarter.png','Preview_Face.png','Preview_Back.png','Preview_Minigun.png','model_report.json','validation_report.json']
for name in files:shutil.copy2(OUT/name,TARGET/name)
for folder in ['Textures','Minigun_Parts']:
    (TARGET/folder).mkdir(exist_ok=True)
    for source in (OUT/folder).iterdir():
        if source.is_file():shutil.copy2(source,TARGET/folder/source.name)
notes=f'''THE GUNNER — DETAYLI LOW POLY

SKM_Gunner_APose.fbx
  A pozunda iskeletli karakter. {stats['character_triangles']:,} üçgen,
  {stats['character_vertices']:,} vertex, {stats['bones']} kemik, tek materyal.
  Boy: {stats['character_height_cm']} cm. En fazla {stats['max_influences']} kemik ağırlığı/vertex.

SM_Gunner_Minigun.fbx
  Karakterden ayrı, tek parça static mesh minigun. {stats['minigun_triangles']:,} üçgen.
  Altı namlu, kimyasal kanallar, yan tüpler, taşıma kolu ve arka tutamak.

Minigun_Parts/
  Namlu grubunu döndürmek için gövde ve namlular ayrıca iki ayrı FBX'tir.
  Namlu parçasının pivotu kendi dönüş eksenindedir.
  Kaynak Blender dosyasında parçalar birleştirilmiş konumlarında durur.
  Blender'da namlu konumu: X=0, Y=-42, Z=2.5 cm. Yerel dönme ekseni Y.

Textures/
  Ortak 512x256 renk atlası: BaseColor, Roughness, Metallic, Emissive.
  Karakter ve silah aynı paleti, her mesh tek materyal kullanır.
  FBX'e dokular gömülüdür; bu klasör ayrıca materyal kurulumu için verildi.
  Roughness ve Metallic lineer veri haritalarıdır.

Gunner_APose.blend: Düzenlenebilir karakter, iskelet, ışıklar ve saklı silah.
Minigun_Source.blend: Görünür silah gövdesi ve ayrı namlu grubu.
Preview_*.png: Gerçek 3D modellerden alınmış renderlar.

İSKELET VE KONTROL
Tek root, parmak kemikleri, IK yardımcıları, twist kemikleri ve 6 saç kemiği.
Önceki AlchemistMage iskeletinin 88 temel kemiğinin referans dönüşümleri korunur.
Manny'ye IK Retargeter ile aktarım için temel adlar/hiyerarşi mevcuttur;
bu model kendi skeleton'u ile kullanılmak üzere hazırlanmıştır.
FBX A pozu içerir. Yeni animasyon klipleri bu pakete eklenmedi.
Idle/Walk/Run/Cast hareketlerinden 20 örnek poz Blender'da kontrol edildi.
FBX yeniden açılarak mesh sayısı, ölçek, kemikler, dokular ve ağırlıklar doğrulandı.
Unreal'a import veya UE içi retarget testi yapılmadı. Teslim FBX olarak bırakıldı.
'''
(TARGET/'OKU.txt').write_text(notes,encoding='utf-8-sig');(OUT/'README.md').write_text(notes,encoding='utf-8')

# Presentation sheet composed only from the actual Blender renders.
canvas=Image.new('RGB',(1800,1400),(22,27,30));d=ImageDraw.Draw(canvas)
fontpath=r'C:\Windows\Fonts\bahnschrift.ttf'
large=ImageFont.truetype(fontpath,46);small=ImageFont.truetype(fontpath,23)
d.text((62,36),'THE GUNNER',font=large,fill=(190,212,68))
d.text((62,94),'LOW POLY  /  A-POSE  /  SKELETAL MESH',font=small,fill=(203,204,191))
def paste_render(name,box):
    im=Image.open(OUT/name).convert('RGB');im.thumbnail((box[2],box[3]),Image.Resampling.LANCZOS)
    canvas.paste(im,(box[0]+(box[2]-im.width)//2,box[1]+(box[3]-im.height)//2))
paste_render('Preview_ThreeQuarter.png',(20,145,970,1170))
d.text((1030,170),'SEPARATE MINIGUN',font=small,fill=(190,212,68))
paste_render('Preview_Minigun.png',(1010,215,760,560))
paste_render('Preview_Face.png',(1105,805,525,525))
d.text((65,1344),f"CHARACTER: {stats['character_triangles']:,} TRIS     MINIGUN: {stats['minigun_triangles']:,} TRIS",font=small,fill=(203,204,191))
canvas.save(TARGET/'Gunner_Preview.png');canvas.save(OUT/'Gunner_Preview.png')
for file in ['SKM_Gunner_APose.fbx','SM_Gunner_Minigun.fbx']:
    assert hashlib.sha256((TARGET/file).read_bytes()).digest()==hashlib.sha256((OUT/file).read_bytes()).digest()
archive=TARGET.with_suffix('.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in TARGET.rglob('*'):
        if p.is_file():z.write(p,str(Path(TARGET.name)/p.relative_to(TARGET)))
print(json.dumps({'delivery':str(TARGET),'archive':str(archive),'character_triangles':stats['character_triangles'],'minigun_triangles':stats['minigun_triangles']},ensure_ascii=False),flush=True)
