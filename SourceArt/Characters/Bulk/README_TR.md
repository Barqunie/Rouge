# The Bulk — Low Poly / UE 5.8

Gönderilen karakter sayfasından modellenmiş tek A pozu karakteri. Referanstaki
aksiyon pozları ve farklı yüz ifadeleri ayrı modeller olarak üretilmedi.
Gövde, gaz maskesi, kayışlar, sırt tankı, hortumlar, eldivenler, kemer şişeleri,
dizlikler ve botlar gerçek 3D mesh'tir. Blender Python ile bu karakter için
modellenmiştir; yeni kas yüzeyleri birleştirilip düşük poligona indirilmiştir.

## Dosyalar

- `SKM_Bulk_APose.fbx`: Unreal için rigli ve ağırlıkları atanmış ana model.
- `SKM_Bulk_APose.glb`: gömülü atlas ve iskelet ile taşınabilir sürüm.
- `Bulk_APose.blend`: düzenlenebilir model, iskelet ve stüdyo ön izleme sahnesi.
- `T_Bulk_Palette.png`: 256 × 256 renk atlası.
- `Preview_ThreeQuarter.png`, `Preview_Front.png`, `Preview_Back.png`: aynı A
  pozunun farklı açılardan görüntüleri; ayrı poz modelleri değildir.

## Bu projede hazır

Content Browser konumu: `/Game/Characters/Bulk`.

`SKM_Bulk`, `SKM_Bulk_Skeleton`, `IK_Manny_ForBulk`, `IK_Bulk`,
`RTG_Manny_to_Bulk`, `M_BK_Body`, `M_BK_Toxic`, `T_Bulk_Palette` ve
`SKM_Bulk_PhysicsAsset` hazırdır. IK Retargeter'da 22 zincir eşleştirilmiştir.
Kaynak model `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.

Manny animasyonlarını aktarmak için `RTG_Manny_to_Bulk` asset'ini aç, animasyonu
seçip ön izle ve Export Retargeted Assets kullan. Karakterin kendi Skeleton
asset'i vardır; Manny iskeletini doğrudan atamak yerine bu retargeter'ı kullan.

## FBX'i yeni projeye aktarma

UE 5.8 klasik FBX importer'ında doğrulanan ayarlar:

1. Skeletal Mesh / Import as Skeletal açık; Skeleton **None**.
2. Import Uniform Scale **1.0**; Convert Scene ve Convert Scene Unit açık.
3. Force Front XAxis **kapalı**, Use T0 As Ref Pose **açık**.
4. Import Normals and Tangents; Import Animations kapalı.
5. Materyal ve texture aktarımını aç; PNG atlas ayrıca da sağlanmıştır.

Mesh, bu projedeki Manny ile aynı +Y yönüne bakar. Character Blueprint Mesh
bileşeni dönüşünü kendi Blueprint'ine göre ayarla. FBX okuyucusu bind-pose
matrislerini yeniden oluşturma uyarısı verebilir; T0 referans pozuyla aktarım
ve sonraki animasyon testleri doğrulandı.

Hazır Unreal asset'lerini başka projeye taşımak için Migrate kullan. ZIP içindeki
`.uasset` dosyaları UE 5.8 içindir; hedef projede Manny referanslarını kontrol et.
FBX aktarımı tek başına IK Rig ve Retargeter asset'lerini oluşturmaz.

## Teknik bilgiler

| Özellik | Değer |
| --- | --- |
| Üçgen | 8,628 |
| Kaynak köşe | 4,782; UV/normal sınırları export sırasında bölebilir |
| Kemik | 91; tek root, Manny çekirdek adları, parmaklar ve IK yardımcıları |
| Ek kemikler | Tank için 1, ön kumaş için 2 |
| Köşe başına ağırlık | En fazla 4; normalize; ağırlıksız köşe yok |
| Boy | 211.9 cm |
| Materyal | 2 |
| Teslim pozu | Tek A pozu, animasyon klibi yok |

FBX tekrar içeri alındı; GLB skin verisi, tek root, kemik sayısı, ağırlıklar ve
eklem sürekliliği kontrol edildi. Sırt tankının şekli bozulmadan kemiğini izlediği
doğrulandı. UE 5.8'de Manny idle/yürüme/koşma aktarımı ile her klipte dört
zamanda kol ve bacak uzunlukları ve bütün kemik dönüşümleri kontrol edildi.
Hareket ön izlemeleri gerçek UE kemik pozlarından Blender'da render edilip
incelendi. Bunlar QA amaçlıdır; aksiyon pozları ve test klipleri teslim ZIP'ine
eklenmedi. Projedeki `Validation` alt klasörü yalnızca bu testler içindir.

Tankın yeşil penceresi opak low-poly malzemedir. Hortumlar skin ağırlıklarıyla
gövdeyi takip eder. Kumaş/boru fizik simülasyonu, yüz blendshape'leri ve özel
saldırı animasyonları içermez. Otomatik Physics Asset için ragdoll ayarı ve
oyunun aşırı hareketlerinde ek deformasyon kontrolü gerekebilir.

Kaynak betikleri: `build_bulk.py`, `validate_model.py`, `setup_unreal.py`,
`render_retarget_previews.py`, `package_delivery.py`. Blender 4.2+ / bpy ve Pillow;
Unreal kurulum betiği UE 5.8 Python ortamında çalışır.

[Epic IK Rig / retarget rehberi](https://dev.epicgames.com/documentation/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine).
