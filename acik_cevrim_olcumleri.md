# Buck dönüştürücü açık çevrim ölçümleri

Tarih: 18 Ekim 2026

- Kart: NUCLEO-F303K8
- PWM: 40 kHz
- PI kontrolü: kapalı
- Başlangıçta bildirilen giriş gerilimi: 12 V; her ölçümde yeniden doğrulanmadı.
- Yük: seri bağlı iki adet 0,47 Ω / 50 W direnç; nominal toplam 0,94 Ω / 100 W.
- Tek multimetreyle akım ve gerilim sırayla ölçüldü. Akım ölçümünde multimetre yükle seri bağlıydı.
- Tablodaki değerler kullanıcının multimetre okumalarıdır; aksi belirtilen ilk kayıtlar ayrıca işaretlenmiştir. Gerilim ve akım eşzamanlı ölçülmediği için güçler yaklaşık karşılaştırma değeridir.

## Ölçüm tablosu

| Duty (%) | Çıkış gerilimi (V) | Çıkış akımı (A) | Yaklaşık güç (W) | Not |
|---:|---:|---:|---:|---|
| 10 | 1,36 | — | — | İlk çıkış gerilimi bildirimi. |
| 10 | 1,34 | — | — | Sonraki multimetre ölçümü. |
| 10 | 1,37 | — | — | ADC kalibrasyonu sonrasındaki ölçüm. |
| 10 | — | 1,15 | — | Seri multimetreyle ölçüm; aynı noktada yüksüz AD_C=2256, yüklü AD_C=2200 bildirildi. |
| 15 | 1,88 | — | — | PA0=0,188 V; AD_V=224. |
| 15 | 1,89 | — | — | Sonraki çıkış gerilimi ölçümü. |
| 24 | 2,81 | 2,25 | 6,32 | Gerilim ve akım sırayla ölçüldü. |
| 34 | — | — | — | Yaklaşık 4 V hedefiyle yüklendi; ölçüm bildirilmedi. |
| 38 | 4,22 | 4,60 | 19,41 | İlk kayıt; sonraki %38 ölçümünden farklı. |
| 40 | 4,40 | 3,90 | 17,16 | Multimetre ölçümleri. |
| 38 | 4,20 | 3,70 | 15,54 | Tekrar ölçümü. |
| 42 | 4,61 | 4,02 | 18,53 | Multimetre ölçümleri. |
| 44 | 4,81 | 4,20 | 20,20 | Multimetre ölçümleri. |
| 46 | 5,00 | 4,37 | 21,85 | Multimetre ölçümleri. |
| 48 | — | 4,56 | — | Seri ampermetre varken çalıştı. Ampermetre çıkarılıp yük doğrudan bağlandığında devrenin kapandığı bildirildi; gerilim ölçülemedi. |

“—”: Ölçüm bildirilmedi. Birbirinden ayrı ölçümler varsayımla birleştirilmedi.

## Ölçüm koşulları ve açık konular

- Ampermetre, sigortası ve kabloları seri direnç ekler. Doğrudan yük bağlantısı ile seri ampermetreli bağlantı aynı çalışma koşulu değildir.
- %48 duty'de kapanmanın nedeni kesinleştirilmedi. Seri direnç kalkınca akımın artması ve beslemenin korumaya geçmesi olası açıklamadır; doğrulanmış sonuç değildir.
- Nominal 0,94 Ω yükte 5 V, yaklaşık 5,32 A gerektirir. Kullanıcının belirttiği dönüştürücü akım sınırı 5 A'dır.
- PA3'te ölçülen sensör gerilimi ile akım ADC sayımı arasında çözülmemiş uyuşmazlık vardır. LCD akım göstergesi güvenilir referans olarak kullanılmamalıdır.
- Son yüklenen ayar: %8 duty, 40 kHz, PI kapalı; yaklaşık 1 V hedefi henüz doğrulanmadı. Önceki %29 ayarı: 12 V giriş ve 1 Ω yük koşulunun devam ettiği varsayımıyla multimetrede 3,31 V, LCD üzerinde 3,4 V bildirildi. Önceki %30 ayarında, 12 V giriş ve 1 Ω yükle 3,4 V ölçüldü.

## Yeni ölçümler

Her yeni kayıtta mümkünse giriş gerilimi, güç kaynağı akım sınırı, yük durumu ve ampermetrenin seri bağlı olup olmadığı da belirtilmelidir.

| Duty (%) | Giriş (V) | Çıkış (V) | Çıkış akımı (A) | Ölçüm bağlantısı / not |
|---:|---:|---:|---:|---|
| 30 | 12,00 | 3,40 | — | Kullanıcının belirttiği 1 Ω yük; akım ölçümü bildirilmedi. |
| 29 | 12,00 | 3,31 | — | LCD: 3,4 V. Önceki giriş/yük koşullarının devam ettiği varsayıldı; akım ölçülmedi. |
| | | | | |

## Gerilim gösterimi kalibrasyonu

- %29 duty koşulunda multimetre: 3,32 V; kalibrasyon öncesi LCD: 3,38 V.
- Gösterim katsayısı 10,5 yerine 10,3136 yapılarak karta yüklendi.
- Katsayı iki ondalığa yuvarlanmış LCD değeriyle hesaplandı; tek nokta düzeltmesidir. Yeni gösterim ve farklı gerilimlerde doğruluk henüz ölçümle doğrulanmadı.


## Kapalı çevrim testine geçiş

1,00 V hedefli, maksimum %12 duty sınırlı PI firmware'i kullanıcı onayıyla karta yüklendi ve doğrulandı. Ölçülen gerilim 1,50 V'u aşarsa PWM durduruluyor. Akıma dayalı koruma yok. Çıkış ölçümü bekleniyor.


## Kapalı çevrim sonuçları

| Hedef (V) | Multimetre (V) | LCD (V) | Not |
|---:|---:|---:|---|
| 1,00 | 1,03 | 1,00 | Kullanıcı kararlı olduğunu bildirdi. |
| 2,00 | 2,02 | 2,00 | Kullanıcı olumlu sonuç bildirdi. Duty sınırı %22, gerilim durdurma eşiği 2,50 V. |
| 3,00 | 3,01–3,02 | 3,00 | Duty sınırı %32, gerilim durdurma eşiği 3,50 V. |
| 4,00 | 4,00–4,01 | 4,00 | Duty sınırı %40, gerilim durdurma eşiği 4,50 V. |
| 5,00 | 4,97–4,98 | 5,00 | Çıkış akımı yaklaşık 4,29 A; hesaplanan güç yaklaşık 21,3 W. Duty sınırı %46, gerilim durdurma eşiği 5,50 V. |


3,00 V kapalı çevrim hedefinde LCD 3,00 V, multimetre 3,01–3,02 V ölçüldü. Duty üst sınırı %32, gerilim durdurma eşiği 3,50 V.


4,00 V kapalı çevrim hedefinde LCD 4,00 V, multimetre 4,00–4,01 V ölçüldü. Duty üst sınırı %40, gerilim durdurma eşiği 4,50 V.


5,00 V kapalı çevrim hedefinde LCD 5,00 V, multimetre 4,97–4,98 V ölçüldü. Duty üst sınırı %46, gerilim durdurma eşiği 5,50 V. Kullanıcı çıkış akımını yaklaşık 4,29 A bildirdi; gerilim ve akım sırayla ölçüldüğünden yaklaşık güç 21,3 W.

