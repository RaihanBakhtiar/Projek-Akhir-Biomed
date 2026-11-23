# ECG CHF-NSR 2D-CNN Project

Paket berisi 8 notebook. Dataset tidak perlu dicari atau disusun manual: notebook 01 mengunduhnya otomatis dari repositori resmi MathWorks `mathworks/physionet_ECG_data`.

## Dataset otomatis
- 66 rekaman ECG yang digunakan dalam pipeline biner
- Sampling rate: 128 Hz
- Panjang tiap rekaman: 65.536 sampel
- CHF: 30 rekaman
- NSR: 36 rekaman
- Rekaman ARR dikecualikan dari manifest dan pipeline; berkas sumber lama tidak dihapus.

Sumber: https://github.com/mathworks/physionet_ECG_data
Dataset ini bersumber dari PhysioNet dan menyertakan file atribusi serta lisensi di dalam arsip.

## Cara menjalankan
1. Install library:
   `pip install -r requirements.txt`
2. Buka folder `notebooks`.
3. Jalankan notebook 01 terlebih dahulu dengan koneksi internet. Notebook mengunduh dan mengekstrak dataset secara otomatis.
4. Jalankan notebook 02 sampai 08 secara berurutan.

## Catatan metodologi
- Notebook 01 menghasilkan `data/processed/manifest.csv` yang hanya memuat CHF/NSR. Berkas raw ARR yang sudah ada tetap tersimpan dan tidak diproses.
- Data dataset sudah 128 Hz dan 65.536 sampel, sehingga notebook 02 hanya memvalidasi dan menyalin data.
- Notebook 03 menggunakan implementasi PSO dengan objektif SURE sebagai pendekatan eksplisit, bukan klaim replikasi identik fungsi objektif jurnal.
- Notebook 05 menyimpan empat subband 2D DWT sebagai empat kanal dalam satu citra 28x28x4 per segmen.
- Notebook 06 melatih model biner di `06_model_chf_nsr/` dan membagi rekaman secara stratified berdasarkan `record_id` untuk mengurangi kebocoran akibat overlap window.
- Notebook 07 melaporkan metrik test per segmen dan per rekaman; jumlah rekaman uji terbatas sehingga hasil bukan jaminan akurasi klinis.
- Notebook 08 menjalankan inferensi di laptop melalui Serial USB. Tidak ada MQTT maupun cloud.
- Hasil serial biner disimpan ke `ecg_classification_results_chf_nsr.csv`, terpisah dari CSV lama tiga kelas.
- Hasil model merupakan eksperimen klasifikasi, bukan diagnosis medis.
