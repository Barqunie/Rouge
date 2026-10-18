# Plague Ghoul — A pozunda rigli karakter

Gönderilen tek görselden uyarlanmış düşük poligonlu karakter. Görselde görünmeyen
arkası ve yan yüzeyleri aynı stil düşünülerek modellenmiştir.

## Bu Rouge projesinde hazır olanlar

Content Browser konumu: `/Game/Characters/PlagueGhoul`

- `SKM_PlagueGhoul`: A pozunda, ağırlıkları atanmış Skeletal Mesh.
- `SKM_PlagueGhoul_Skeleton`: karakterin kendi iskeleti.
- `IK_Manny_ForPlagueGhoul`: projedeki Manny için kaynak IK Rig.
- `IK_PlagueGhoul`: karakterin hedef IK Rig'i; otomatik FBIK kurulumu uygulandı.
- `RTG_Manny_to_PlagueGhoul`: 22 zinciri eşleştirilmiş IK Retargeter.
- `Animations/PG_MM_Idle_Verified`: Manny'nin MM_Idle animasyonundan aktarılıp ön izlemesi kontrol edilen örnek.
- `M_PG_Body`, `M_PG_Toxic`, `T_PlagueGhoul_Palette`: atanmış materyaller ve renk atlası.

Retargeter'ı aç, Asset Browser'dan Manny animasyonunu seç ve karakter üzerindeki
sonucunu ön izle. İstediğin animasyonları Export Retargeted Assets ile üret.
Bu kurulumun kaynak mesh'i `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.
Başka bir projede bu yol farklıysa kaynak IK Rig'in Preview Mesh alanını güncelle.

Karakterin oranları Manny'den farklıdır; kendi Skeleton asset'ini kullanır.
Manny'nin Skeleton asset'ini doğrudan atamak yerine verilen IK Retargeter ile aktar.
Başın çene kemiğine bakarak yanlış hizalanmasını önlemek için retarget pozu yalnızca
ana kol ve bacak kemiklerinde otomatik hizalanmıştır. Bind pose A pozu olarak kalır.

## Dosyalar

- `SKM_PlagueGhoul_APose.fbx`: Unreal için skinned mesh ve iskelet. Animasyon klibi içermez.
- `SKM_PlagueGhoul_APose.glb`: gömülü renk atlasıyla taşınabilir rigli model.
- `PlagueGhoul_APose.blend`: düzenlenebilir model, iskelet ve ön izleme ışık/kamera sahnesi.
- `T_PlagueGhoul_Palette.png`: 256 × 256 renk atlası.
- `Preview_ThreeQuarter.png`, `Preview_Front.png`, `Preview_Back.png`: A pozu görüntüleri.
- `Preview_RigTest.png`: dirsek, diz ve parmak bükülme kontrolü.
- `Unreal_APose.png`, `Unreal_RetargetedIdle.png`: Unreal'dan alınan doğrulama görüntüleri.

## FBX'i başka bir projeye aktarma

Unreal 5.8'in klasik FBX importer'ında doğrulanan ayarlar:

1. Skeletal Mesh / Import as Skeletal açık; Skeleton seçimi **None**.
2. Import Uniform Scale **1.0**, Convert Scene ve Convert Scene Unit açık.
3. **Force Front XAxis** ve **Use T0 As Ref Pose** açık.
4. Import Normals and Tangents; Import Animations kapalı.
5. Import Materials ve Import Textures açık. Renk atlasını gerektiğinde ayrıca içeri al.

Unreal'ın FBX okuyucusu bind-pose matrisleri için yeniden oluşturma mesajı verebilir.
T0 referans pozu ile içeri aktarım ve sonraki animasyon aktarımı bu projede doğrulandı.
Verilen GLB ve Blender dosyaları aynı A pozundaki modeli içerir.

`setup_unreal.py` dosyası, Unreal Python ortamında mesh/material/IK Rig/Retargeter
kurulumunu tekrar oluşturmak için kaynak olarak sağlanmıştır. UE 5.8 klasik FBX
importer'ıyla çalıştırılmıştır; farklı Unreal sürümlerinin Python API'leri değişebilir.
Mevcut projedeki hazır asset'ler için bu betiği yeniden çalıştırmak gerekmez.

## Teknik bilgiler ve kontrol kapsamı

| Özellik | Değer |
| --- | --- |
| Üçgen | 4.895 (Blender kaynak mesh) |
| Kaynak köşe | 2.692; düz yüzey normalleri/UV dikişleri dışa aktarımda köşe sayısını artırır |
| Kemik | 88; tek `root`, pelvis, 5 spine, 2 neck, head, jaw, eller/parmaklar ve IK yardımcıları |
| Ağırlık | Köşe başına en fazla 2; normalize; ağırlıksız köşe yok |
| Boy | Yaklaşık 278,5 cm |
| Materyal | 2 |
| Bind pose | A pozu; kollar omuz çizgisinden yaklaşık 45° aşağıda |

FBX tekrar içeri alınarak tek root, 88 kemik ve skinning kontrol edildi. Blender'da
eklem/parmak bükülmeleri görsel olarak incelendi. Unreal'da Skeletal Mesh olarak
açılışı, materyaller ve Manny idle aktarımı kontrol edildi. Bütün oyun animasyonları
için kapsamlı bir deformasyon testi yapılmadı; özellikle aşırı hareketlerde poz
ve ağırlık rötuşları gerekebilir. Physics Asset/ragdoll bu pakette kurulmadı.

Yeniden üretim: Blender 4.2+ Python veya `bpy` ve Pillow bulunan Python ile
`build_ghoul.py`; dosya ve rig kontrolü için `validate_model.py`.

Epic'in retarget akışı: https://dev.epicgames.com/documentation/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine
