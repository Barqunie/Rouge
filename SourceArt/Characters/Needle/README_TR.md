# The Needle — Unreal Engine 5.8

Gönderilen The Needle / Toxic Assassin görselinden uyarlanmış düşük poligonlu
3D karakter. Mesh, Blender Python ile bu karakter için modellenmiştir. Görselde
görünmeyen arka ve yan yüzeyler yorumlanmıştır. Model A pozunda, gerçek mesh,
iskelet, UV ve skin ağırlıkları içerir.

## Hangi dosyayı kullanmalıyım?

- `SKM_Needle_APose.fbx`: silahsız ana Skeletal Mesh. Oyun sırasında silahları
  takıp çıkaracaksan bunu kullan.
- `SKM_Needle_Equipped_APose.fbx`: iki hançer dahil sürüm. Hançerler el altındaki
  `weapon_l` ve `weapon_r` kemiklerine bağlıdır. Tek Skeletal Mesh olarak aktarılır.
- `SM_Needle_Dagger.fbx`: bağımsız hançer Static Mesh. Pivotu kabzanın merkezindedir.
  Blender kaynakta bıçak yerel +Y yönündedir. Unreal'da el veya `weapon_l/r`
  kemiğine takarken socket/relative transform ile yönünü ayarla.
- `T_Needle_Palette.png`: iki materyalin kullandığı 256 × 256 renk atlası.
- `SKM_Needle_APose.glb`: atlası gömülü, rigli ana model.
- `Needle_APose.blend`: düzenlenebilir sahne; silahlar ayrı nesnelerdir.

FBX dosyaları bind pose içerir, animasyon klibi içermez. Retarget edilmiş örnek
animasyonlar paketin `Content/Characters/Needle/Animations` klasöründedir.

## Rouge projesinde hazır kurulum

Content Browser: `/Game/Characters/Needle`

- `SKM_Needle` ve `SKM_Needle_Equipped`: aynı `SKM_Needle_Skeleton` iskeletini kullanır.
- `IK_Manny_ForNeedle` ve `IK_Needle`: kaynak ve hedef IK Rig.
- `RTG_Manny_to_Needle`: Manny'den bu karaktere 22 zincir eşleştirilmiş retargeter.
- `Animations/ND_MM_Idle_Verified`: idle örneği.
- `Animations/ND_MF_Unarmed_Walk_Fwd_Verified`: yürüme örneği.
- `Animations/ND_MF_Unarmed_Jog_Fwd_Verified`: koşma örneği.
- `M_ND_Body`, `M_ND_Toxic`, `T_Needle_Palette`: atanmış materyaller ve atlas.
- `SM_Needle_Dagger`: ayrı silah mesh'i.

`RTG_Manny_to_Needle` asset'ini açıp Asset Browser'da Manny animasyonunu seç.
Sonucu kontrol ettikten sonra Export Retargeted Assets ile kendi animasyonlarını
üret. Kaynak model bu projedeki `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.
Silahlı sürüm aynı iskeleti kullandığı için bu animasyonları da oynatabilir.
Bu örnekler silahsız Manny klipleridir; hançerli sürümde doğru kavrama ve bıçakların
gövdeye girmemesi için uygun silah animasyonu veya el/bilek pozu gerekir.

Bu karakterin kendi iskeleti vardır. Manny'nin Skeleton asset'ini doğrudan
seçmek yerine IK Retargeter ile animasyon aktar. Omuz, dirsek, el, parmak, kalça,
diz ve ayak kemikleri Manny çekirdek isimlendirmesini kullanır.

## Başka bir UE 5.8 projesine FBX aktarımı

Klasik FBX importer'ında doğrulanan ayarlar:

1. Ana FBX için Skeletal Mesh / Import as Skeletal açık, Skeleton **None**.
2. Import Uniform Scale **1.0**; Convert Scene ve Convert Scene Unit açık.
3. **Force Front XAxis kapalı**, **Use T0 As Ref Pose açık**.
4. Import Normals and Tangents; Import Animations kapalı.
5. Import Materials ve Import Textures açık. Atlas FBX'e gömülüdür ve ayrıca PNG olarak bulunur.
6. İkinci, silahlı FBX'i aktarırken ilk adımda oluşan `SKM_Needle_Skeleton` seçilebilir.
7. Ayrı hançer FBX'ini **Static Mesh** olarak içeri al.

Mesh yönü projedeki Manny ile aynı +Y'dir. Character Blueprint Mesh bileşenindeki
dönüşü kullandığın karakter Blueprint'ine göre ayarla. FBX okuyucusunun bind-pose
matrisleriyle ilgili uyarısı T0 referans pozu ile yeniden oluşturularak giderilir;
bu ayarla UE 5.8 içeri aktarımı ve animasyon aktarımı doğrulandı.

ZIP içindeki `.uasset` dosyaları UE 5.8'de oluşturuldu. Başka projeye hazır
kurulumu taşımak için Unreal'ın Migrate işlemini kullan; retargeter'ın kaynak
Manny/IK Rig referanslarını hedef projede kontrol et. Ham FBX'ten aktarım yapmak
hazır IK Rig ve retargeter asset'lerini otomatik oluşturmaz.

## Ölçüler ve testler

| Özellik | Değer |
| --- | --- |
| Ana model | 4.236 üçgen / 2.382 kaynak köşe |
| İki hançerli model | 4.686 üçgen |
| Hançer | 225 üçgen |
| Kemik | 99, tek `root` |
| Ek kemikler | 9 pelerin, 2 silah; el/parmak ve IK yardımcıları dahil |
| Ağırlık | Köşe başına en fazla 2; normalize; ağırlıksız köşe yok |
| Materyal | 2 |
| Boy | Yaklaşık 189,2 cm |
| Üst kol / ön kol | 30,46 cm / 27,73 cm |
| Bind pose | A pozu, kollar yaklaşık 45 derece aşağıda |

Üç FBX dosyası Blender'a tekrar alınarak mesh, iskelet, root ve skinning kontrol
edildi. GLB'nin skin, joint ve ağırlık verileri kontrol edildi. Dirsek, diz,
parmak ve pelerin bükülme testi yapıldı; hançerlerin el hareketini izlediği
doğrulandı. Unreal'da idle, yürüme ve koşma aktarılıp her klipte dört zamanda
kemik dönüşümleri, kol/bacak uzunlukları ve silah kemiği bağlantıları kontrol
edildi. Ön izlemeler gerçek Unreal animasyonlarından örneklenen kemik pozlarının
Blender'da render edilmesiyle üretildi; UE ekran görüntüsü değildir.

Pelerin kemiklidir; otomatik cloth simulation veya AnimDynamics kurulumu içermez.
İstenirse `cape_l/c/r_01..03` zincirlerine hareket/çarpışma eklenebilir.
Physics Asset otomatik oluşturulmuştur; ragdoll ve oyun içindeki uç saldırı
pozları için ayrıca ayar gerekir. Yüz animasyonu / blendshape içermez.

## Kaynakları yeniden üretmek

Blender 4.2+ Python veya `bpy` ve Pillow kurulu Python ortamında `build_needle.py`.
Dosya kontrolü için `validate_model.py`. UE 5.8 Python ortamında
`setup_unreal.py` yalnızca `/Game/Characters/Needle` altındaki kurulumu oluşturur
veya yeniler. `render_retarget_previews.py` gerçek UE test pozlarını render eder.

Retarget yöntemi için [Epic'in IK Rig rehberi](https://dev.epicgames.com/documentation/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine).
