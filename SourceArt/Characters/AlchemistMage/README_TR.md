# Alchemist Mage — Low Poly / UE 5.8

Gönderdiğin referans esas alınarak Blender Python ile oluşturulmuş gerçek 3D
karakter. Detaylar sadeleştirildi: bordo saç/topuz, yeşil mercek, beyaz ceket,
korse, uzun botlar ve iksirler korundu. Referansın birebir otomatik 3D taraması
değildir. Modelin bind pozu A pozudur; boyu topuz dahil 192.7 cm.

## Hemen kullan

Bu Rouge projesinde `/Game/Characters/AlchemistMage` klasörü hazır.

- Ana Skeletal Mesh: `SKM_AlchemistMage`
- Kendi iskeleti: `SKM_AlchemistMage_Skeleton`
- Retargeter: `RTG_Manny_to_AlchemistMage`
- IK Rig'ler: `IK_Manny_ForAlchemistMage`, `IK_AlchemistMage`
- Klipler: `Animations/AM_Idle`, `AM_Walk`, `AM_Run`, `AM_Cast`

Karakterin oranlarına özel bir Skeleton kullanılır. Manny animasyonlarını
aktarmak için verilen IK Retargeter'ı kullan; mesh importunda doğrudan Manny
Skeleton'ını seçme. 22 retarget zinciri eşleştirildi. Kaynak Manny mesh'i:
`/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.

## FBX dosyaları

| Dosya | İçerik |
| --- | --- |
| SKM_AlchemistMage_APose.fbx | A pozunda skinli mesh + 106 kemik |
| AM_Walk.fbx | 1,50 sn yürüme döngüsü |
| AM_Run.fbx | 1,77 sn koşma döngüsü |
| AM_Cast.fbx | 1,30 sn sağ elle büyü fırlatma; tek seferlik |
| AM_Idle.fbx | 7,57 sn bekleme döngüsü; ek klip |

Tüm klipler **30 fps ve in-place**: ileri hareketi oyundaki Character Movement
sağlar. Walk/Run/Idle mevcut projedeki Manny kliplerinden bu iskelete retarget
edildi, saç/ceket hareketleri eklendi. Cast ayrıca pozlanıp bake edildi:
hazırlık, kolu geriye çekme, öne atış, takip hareketi ve beklemeye dönüş.

**Büyü çıkışı: 0,60 saniye** (FBX/Blender'da 19. kare; UE sıfırdan sayarsa 18).
`AM_Cast` üzerinde bu zamana bir `CastRelease` Anim Notify ekleyip projectile
veya Niagara efektini oyunun Blueprint'inden başlatabilirsin. Elden çıkış için
`hand_r` kemiğine socket eklenebilir. Klip animasyon içerir; projectile, hasar,
VFX, Anim Blueprint veya saldırı montage mantığı eklenmedi.

## Yeni UE 5.8 projesine import

1. Önce `SKM_AlchemistMage_APose.fbx`: Skeletal Mesh açık, Skeleton **None**,
   Import Animations kapalı.
2. Scale **1.0**, Convert Scene/Convert Scene Unit açık, Force Front XAxis
   kapalı, Use T0 As Ref Pose açık, Import Normals and Tangents.
3. Animasyon FBX'lerini ayrı içeri al: Import Mesh kapalı, Import Animations
   açık, Skeleton olarak az önce oluşan `SKM_AlchemistMage_Skeleton` seçili.
   Exported Time ve 30 fps kullan. Root Motion kapalı.
4. `T_AlchemistMage_Palette.png` atlasını materyallerin Base Color girişine
   bağla. Toxic slotunda istenirse hafif Emissive kullan. Hazır UE asset'lerinde
   malzemeler zaten bağlanmıştır. Kumaş için Two Sided açık tutulur.

Mesh bu projedeki Manny gibi +Y yönüne bakar. Character Blueprint Mesh
bileşenindeki dönüşü mevcut Blueprint'ine göre ayarla. FBX okuyucusu bind pose
matrislerini yeniden oluşturma uyarısı verebilir; T0 ile import ve sonraki tüm
animasyon kareleri gerçek UE 5.8 içinde doğrulandı.

Hazır `.uasset` dosyalarını başka projeye taşımak için UE'nin Migrate işlemini
kullan. ZIP'teki Content yolu korunmuştur; IK Retargeter için Manny kaynak
referanslarının hedef projede bulunması gerekir. FBX tek başına IK Rig /
Retargeter asset'lerini oluşturmaz.

## Teknik bilgiler ve kaynak

- 5,898 üçgen, 3,363 kaynak köşe, 2 materyal.
- 106 kemik: Manny çekirdek adları, parmaklar ve IK yardımcıları; ayrıca
  saç için 6 ve ceket için 12 kemik.
- Tek `root`; köşe başına en fazla 2 normalize ağırlık.
- `AlchemistMage_APose.blend`: düzenlenebilir bind mesh / rig.
- `AlchemistMage_Animated.blend`: dört Action ve düzenlenebilir animasyon.
- `SKM_AlchemistMage_APose.glb`: gömülü atlasla taşınabilir rigli sürüm;
  bu GLB bind modeldir, klipler FBX ve animated Blend içindedir.
- `Preview_*.gif`: gerçek mesh ile render edilen hareket ön izlemeleri.

Saç ve ceket hareketleri bu kliplere bake edildi. Fizik/cloth simülasyonu
kurulmadı; başka animasyonlarda bu ek kemikleri Control Rig / AnimDynamics
ile hareket ettirebilirsin. Otomatik Physics Asset vardır; ragdoll ayarları
özel olarak düzenlenmedi. Yüz blendshape'leri ve referanstaki şırınga silahı
bu sadeleştirilmiş sürümde bulunmaz.

FBX tekrar importu, GLB skin verisi, eklemler, ağırlıklar ve döngü uçları
doğrulandı. Dört klipte toplam 368 kare UE 5.8'de okunup Blender pozlarıyla
karşılaştırıldı; en büyük kemik konumu farkı
0.0075 cm.
`Validation` klasöründeki ham retarget testleri pakete dahil edilmez.

Yeniden üretim sırası: `build_mage.py` (bpy 4.2+ / Pillow), `setup_unreal.py`
(UE Python), `animate_mage.py` (bpy / NumPy), `validate_model.py`,
`import_animations_unreal.py` (UE Python), `render_animation_previews.py`.
Kaynak betikleri mevcut Rouge projesinin Manny animasyonlarını kullanır.
