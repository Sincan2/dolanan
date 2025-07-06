#!/bin/bash

# ================================================================= #
#  Skrip Manajemen Klien ZeroTier dengan Menu di Ubuntu           #
# ================================================================= #
#                      Dibuat oleh Hadi                             #
#         (Dimodifikasi dengan penambahan menu)         #
# ================================================================= #

# Periksa apakah skrip dijalankan sebagai root
if [ "$(id -u)" -ne 0 ]; then
  echo "❌ Skrip ini harus dijalankan sebagai root. Coba jalankan dengan 'sudo ./znet.sh'"
  exit 1
fi

# Fungsi untuk memeriksa status dan memulai ZeroTier
start_zerotier() {
  if ! systemctl is-active --quiet zerotier-one; then
    echo "🔧 Memulai layanan ZeroTier..."
    systemctl start zerotier-one
  fi
  # Pastikan service berjalan saat boot
  systemctl enable zerotier-one >/dev/null 2>&1
}

# Periksa apakah ZeroTier sudah terinstal sebelum menampilkan menu
if ! command -v zerotier-cli &> /dev/null; then
  echo "🟡 ZeroTier tidak ditemukan. Memulai instalasi..."
  # Unduh dan jalankan skrip instalasi resmi ZeroTier
  curl -s https://install.zerotier.com | bash
  if [ $? -ne 0 ]; then
    echo "❌ Gagal menginstal ZeroTier. Silakan coba instalasi manual."
    exit 1
  fi
  echo "✅ Instalasi ZeroTier berhasil."
fi

# Mulai layanan ZeroTier jika belum berjalan
start_zerotier

# Loop utama untuk menampilkan menu
while true; do
  echo ""
  echo "========================================="
  echo "         MENU MANAJEMEN ZEROTIER         "
  echo "========================================="
  echo "1. Gabung ke Jaringan (Add ID)"
  echo "2. Keluar dari Jaringan (Hapus ID)"
  echo "3. Tampilkan Daftar Jaringan"
  echo "4. Keluar dari Skrip"
  echo "========================================="
  read -p "Pilih opsi [1-4]: " PILIHAN

  case $PILIHAN in
    1)
      # --- OPSI 1: GABUNG JARINGAN ---
      read -p "Masukkan Network ID untuk bergabung: " NETWORK_ID
      if [ -z "$NETWORK_ID" ]; then
        echo "❌ Network ID tidak boleh kosong."
        continue
      fi

      echo "🚀 Mencoba bergabung ke jaringan: ${NETWORK_ID}"
      JOIN_RESULT=$(zerotier-cli join ${NETWORK_ID})

      if [[ "$JOIN_RESULT" == *"200 join OK"* ]]; then
        echo "✅ Berhasil mengirim permintaan untuk bergabung ke jaringan ${NETWORK_ID}."
        echo "🔔 PENTING: Anda perlu menyetujui (authorize) klien ini dari panel kontrol ZeroTier Anda (my.zerotier.com)."
        echo "================================================================="
        zerotier-cli listnetworks
        echo "================================================================="
      else
        echo "❌ Gagal bergabung ke jaringan. Pesan error:"
        echo "$JOIN_ROW_RESULT"
      fi
      ;;

    2)
      # --- OPSI 2: KELUAR DARI JARINGAN (HAPUS ID) ---
      read -p "Masukkan Network ID yang akan dihapus: " NETWORK_ID
      if [ -z "$NETWORK_ID" ]; then
        echo "❌ Network ID tidak boleh kosong."
        continue
      fi

      # Periksa apakah benar-benar terhubung ke jaringan ini
      if ! zerotier-cli listnetworks | grep -q "$NETWORK_ID"; then
          echo "🟡 Anda tidak terhubung ke jaringan ${NETWORK_ID}."
          continue
      fi

      echo "👋 Mencoba keluar dari jaringan: ${NETWORK_ID}"
      LEAVE_RESULT=$(zerotier-cli leave ${NETWORK_ID})

      if [[ "$LEAVE_RESULT" == *"200 leave OK"* ]]; then
        echo "✅ Berhasil keluar dari jaringan ${NETWORK_ID}."
      else
        echo "❌ Gagal keluar dari jaringan. Pesan error:"
        echo "$LEAVE_RESULT"
      fi
      ;;

    3)
      # --- OPSI 3: TAMPILKAN DAFTAR JARINGAN ---
      echo "🌐 Menampilkan daftar jaringan ZeroTier yang terhubung..."
      echo "================================================================="
      zerotier-cli listnetworks
      echo "================================================================="
      ;;

    4)
      # --- OPSI 4: KELUAR ---
      echo "👋 Terima kasih telah menggunakan skrip ini."
      exit 0
      ;;

    *)
      # --- OPSI TIDAK VALID ---
      echo "❌ Pilihan tidak valid. Silakan pilih nomor dari 1 sampai 4."
      ;;
  esac

  read -p "Tekan [Enter] untuk kembali ke menu..."
done
