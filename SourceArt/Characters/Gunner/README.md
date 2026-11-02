THE GUNNER — DETAYLI LOW POLY

SKM_Gunner_APose.fbx
  A pozunda iskeletli karakter. 14,790 üçgen,
  7,990 vertex, 94 kemik, tek materyal.
  Boy: 188.43 cm. En fazla 2 kemik ağırlığı/vertex.

SM_Gunner_Minigun.fbx
  Karakterden ayrı, tek parça static mesh minigun. 5,048 üçgen.
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
