# Kapalı çevrim firmware ve test özeti

Durum: Firmware 1–5 V hedeflerinde karta yüklenerek denendi; flash doğrulaması başarılı. Güncel kaynak ve kart hedefi 5,00 V kapalı çevrimdir.

## Kontrol ayarları

| Ayar | Değer |
|---|---:|
| Güncel hedef | 5,00 V |
| Hedef artış hızı | 2 V/s; hedefe 0,5 saniyede ulaşır |
| Başlangıç PWM | %0 |
| Güncel duty sınırı | %0–46 |
| Kp | 0,03 duty/V |
| Ki | 0,20 duty/(V·s) |
| Gerilim katsayısı | 10,3136 |
| Hesapta ADC referansı | 3,3 V |
| Kontrol tetiklemesi | TIM2, 1 kHz |
| PWM | TIM1, 40 kHz; 1 yüzde puanı çözünürlük |
| Güncel yazılımsal aşırı gerilim durdurması | Kalibre edilmiş ölçüm >5,50 V |
| LCD yenilemesi | 500 ms |

PI hatası volt cinsinden, çıkışı duty oranı cinsindendir. İntegral bir kez alınır; doyuma doğru büyümesi engellenir. Gerilim kontrolü 4 örnek ortalaması kullanır; LCD'nin güçlü yumuşatma filtresi kontrol geri beslemesine uygulanmaz.

TIM2 kesmesi gerilim ve akım ADC'sinin tek sahibidir. Ana döngü yalnızca kısa bir atomik kopya alarak ekranı günceller. Böylece LCD beklemeleri kontrolü durdurmaz. SysTick daha yüksek önceliktedir; ADC polling zaman aşımı kesmede de ilerler.

## Hata durumları

- FAULT:1: ADC başlatma/okuma/durdurma hatası.
- FAULT:2: Ölçülen gerilim 1,50 V'u aştı.
- FAULT:3: Kontrol aralığı 2 ms'yi veya ölçülen kesme süresi 800 µs'yi aştı.
- FAULT:4: Genel HAL başlatma hatası.

Hata halinde PWM karşılaştırması sıfırlanır ve TIM1 ana çıkışı kapatılır. Otomatik yeniden başlatma yoktur. Genel başlatma hatası sonsuz döngüde kalır; ekran güncellenmeyebilir.

## Yapılan kontroller

- ARM derleme ve bağlama başarılı. Kaynak derlemesinde uyarı yok; önceki sürümlerde de bulunan RWX linker segmenti uyarısı sürüyor.
- PI denklemlerinin davranış kontrolünde sıfır başlangıç, %12 doyum sınırı, negatif hatayla integralin boşalması ve kilitlenen gerilim hatası kontrol edildi. Bu kontrol gerçek güç devresinin simülasyonu veya donanım testi değildir.
- Saat bölücüsü: mevcut 64 MHz TIM2 saatinden 1 MHz sayaç, 1000 sayım ile 1 kHz.
- LCD ile kontrol aynı gerilim katsayısını kullanıyor.

## Doğrulanmamış noktalar

- Ölçüm tablosu ve açık çevrim testleri `docs/bench-results.md` dosyasındadır.
- Kp/Ki başlangıç test değerleridir; kararlılık ve geçici rejim donanımda doğrulanmadı.
- %12 sınırı bir akım limiti değildir. Akım ADC uyuşmazlığı çözülmediğinden akıma dayalı koruma eklenmedi; akım harici izlenmelidir.
- Aşırı gerilim durdurması aynı ADC ölçümüne bağlı yazılım denetimidir; bağımsız donanım koruması değildir.
- PWM çözünürlüğü %1 olduğundan düşük gerilimde küçük salınımlar olabilir.
- control_max_cycles / SystemCoreClock, en uzun gözlenen kontrol kesmesi süresini verir; henüz kartta ölçülmedi.
- TIM2 kontrol kurulumu USER CODE bölgesinde elle yazılmıştır; .ioc içinde ayrılmış bir timer olarak tanımlı değildir. İleride TIM2 başka işe atanmamalı.

## Geri dönüş

review/open_loop_8pct.elf: Son karta yüklenen %8 açık çevrim firmware'inin kopyası.
review/open_loop_8pct_main.c.txt: Değişiklik öncesi main.c kopyası.
Debug/ klasörü üretilen build çıktısıdır ve Git deposunda tutulmaz.
