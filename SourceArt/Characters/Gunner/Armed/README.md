GUNNER — MINIGUN TUTUŞU VE SİLAHLI ANİMASYONLAR

ANA MODEL
SKM_Gunner_Armed_APose.fbx: Karakter ve eldeki minigun aynı skeletal mesh'tedir.
98 kemik, 19.838 üçgen, tek materyal. Önceki 94 kemikli karaktere 4 silah
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
Sağ el arka tetik tutamağını, sol el üst taşıma kolunu kavrar.
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
