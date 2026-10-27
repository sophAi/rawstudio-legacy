# Rawstudio Legacy

[![Platform](https://img.shields.io/badge/Platform-Linux%20Mint%20%7C%20Ubuntu%20%7C%20Debian-brightgreen.svg)]()
[![GTK Version](https://img.shields.io/badge/GUI-GTK%2B%203-blue.svg)]()
[![License](https://img.shields.io/badge/License-GPL--2.0--or--later-orange.svg)]()

**Rawstudio Legacy** 是專門為現代 Linux 發行版（特別是 **Linux Mint 22.3 MATE / Cinnamon** 與 **Ubuntu 24.04 LTS Noble Numbat**）優化與重構的經典開源數位相機 RAW 照片解碼與處理工具。

本專案保留了 Rawstudio 經典的高速預覽、32 位元浮點即時影像處理管線及直覺的批次處理能力，同時徹底解決了在現代 Linux 系統上編譯與運行所面臨的各項技術斷層。

---

## 🌟 現代化重構與修復亮點

### 1. 徹底移除 GConf 依賴，移植至 GLib 原生 `GKeyFile`
* **問題**：GNOME 2 時代的 `gconf-2.0` 在 Ubuntu 20.04+、Linux Mint 21+ 及 Debian 11+ 已被徹底淘汰移除，導致舊版 Rawstudio 無法安裝或編譯。
* **解決方案**：以 GLib 原生 `GKeyFile` 完全重寫 [`librawstudio/conf_interface.c`](file:///home/xanadu/rsync_data/geminicli/rawstudio-legacy/librawstudio/conf_interface.c)。
  * **零外部依賴**：無須任何守護行程（gconfd）或過時相依套件。
  * **符合 XDG 規範**：使用者設定檔直接持久化儲存於純文字檔案 `~/.config/rawstudio-legacy/preferences`（支援舊版 `~/.config/rawstudio/` 自動轉移）。

### 2. 對齊最新 `liblensfun` 0.3.x 標頭檔路徑與校正邏輯修復
* **路徑適配**：自動辨識並對齊 `/usr/include/lensfun/lensfun.h` 與 `pkg-config` 宣告。
* **重大隱蔽 Bug 修復**：原專案在 `lensfun-version.c` 中於 include 標頭檔前就檢查 `#ifdef LF_VERSION`，導致巨集永遠未載入而退回執行期字串解析；加上現代動態庫檔名為 `liblensfun.so.0.3.4`（3段）而非舊版 4 段格式，導致原版誤判版本為 0，引發色差（TCA）反轉校正計算錯誤。此版本已全面修復，確保光學畸變與色差修正公式精準無誤。

### 3. 相機感測器資料庫重大升級（2023 Darktable RawSpeed）
* 核心解碼代碼維持高度穩定的 Klaus Post RawSpeed C++ 引擎。
* 感光元件定義庫更新為 **Darktable 2023** 最新版本（`cameras.xml` 體積自 346 KB 翻倍至 712 KB），為軟體注入數百款現代單反與無反微單相機（Sony A7 系列、Nikon Z 系列、Canon EOS R/RP/M、Fuji X 系列等）的色彩矩陣、黑白電平與感光器裁切參數。

### 4. 內建雙引擎備援：完整整合 dcraw 9.28 與 UFRaw 0.23
* 補回並升級 `load-dcraw` 模組，完整納入 Dave Coffin 的 **dcraw 9.28** 與 **UFRaw 0.23** 引擎（含白平衡預設值與 Nikon 曲線庫）。若遇特定特殊格式（如老舊機型或特殊感光元件），可作為 RawSpeed 之外的強大備援解碼器。

### 5. 適配現代 Exiv2 0.28+ API 變更
* 支援 Exiv2 0.28 智慧指標 (`std::unique_ptr` / `UniquePtr`) 與異常類別 (`Exiv2::Error`)，同時向下相容 Exiv2 0.27。

### 6. 開源授權合規化（SPDX Compliance）
* 針對 Sigma X3F 解碼部分，以 Google Chromium V8 專案（BSD-3-Clause）的 `ConvertUTF16toUTF8` 替換被現代 Linux 社群標註為 Non-free 的舊 Unicode 代碼。

### 7. 程式更名為 `rawstudio-legacy`，平滑並存
* 應用程式二進位檔更名為 `rawstudio-legacy`，並提供相容符號連結 `rawstudio`。
* 桌面捷徑為 `Rawstudio Legacy`，獨立運作且不與發行版既有套件衝突。

### 8. Panasonic RW2 Bayer CFA 解碼色彩偏差修復（消除紫色/洋紅色偏）
* **問題**：舊版 RawSpeed 引擎在 `Rw2Decoder.cpp` 中將 Bayer 矩陣硬編碼為 BGGR，導致 Panasonic 相機（如 DMC-GX1、GF 系列等）照片之綠色通道被誤判為藍/紅通道，解碼後照片呈現全片深紫/洋紅色偏差。
* **解決方案**：依據 RW2 EXIF 標頭之 TIFF Tag `0x0009` (`PANASONIC_CFAPATTERN`) 動態判斷真實 Bayer 陣列排列（如 DMC-GX1 之 `GRBG`），校正 Demosaic 去馬賽克演算法，使色彩還原真實自然。

### 9. 預覽畫布邊界安全與 Pixman 警告消除
* 解決 GTK+ 3 畫布在初次初始化或未分配尺寸時繪製空白背景引發的 `pixman_region32_init_rect: Invalid rectangle passed` 警告與效能損耗。

### 10. TIFF 元數據縮圖緩衝區溢位修復
* 修正 `plugins/meta-tiff/tiff-meta.c` 在搜尋 Panasonic 嵌入式縮圖 JPEG EOI (`0xFFD9`) 時越界讀取記憶體引發的 Segmentation Fault (SIGSEGV) 當機問題。

### 11. Lensfun 鏡頭資料庫自動辨識與光學校正修復
* **Panasonic / Micro 4/3 鏡頭焦距與光圈解析**：修正 `plugins/meta-tiff/tiff-meta.c` 與 `rs-metadata.c`，自動從 `fixed_lens_identifier`（如 `LUMIX G 20/F1.7`）或 EXIF 解析最小/最大焦距與光圈值，消除焦距未設定之問題。
* **解決鏡頭選單 `"-1mm"` 搜尋 Bug**：修正 `rs-lens-db-editor.c` 在焦距未設定時錯誤產生 `"-1mm"` 查詢字串，導致 Lensfun 鏡頭庫過濾失敗、無法列出 Lumix G 20mm f/1.7 Asph. 等鏡頭的缺陷。
* **智慧自動綁定與校正啟用**：新載入 RAW 照片時，於 `rs-lens.c` 自動根據相機機型與鏡頭識別碼比對 Lensfun 資料庫，自動配對並啟用光學修正。
* **完整保留 Lensfun 色差 (TCA) 與暗角校正資料**：修正 `plugins/lensfun/lensfun.c`，當使用者手動微調滑桿為 0 時不再錯誤清除 Lensfun 資料庫中內建的色差與 20 組暗角校正多項式參數，確保畸變、色差、暗角三項修正全效發揮作用。

### 12. 解決初次載入切換照片凍結與關閉卡頓問題 (I/O Lock & Shutdown Clean Exit)
* **消除切換照片凍結 (I/O Lock 死鎖修復)**：修復 `plugins/meta-tiff/tiff-meta.c` 在讀取 EXIF 元數據時遺漏的 `rs_io_unlock()`，防止所有背景縮圖/中繼資料執行緒在 `io_lock` 互斥鎖永久死鎖，讓初次開啟未建立快取目錄時切換照片依然流暢靈敏。
* **徹底解決關閉時卡在進度條問題**：新增 `rs_io_idle_cancel_all()` 函式並重構 `src/rs-actions.c` 之 `ACTION(quit)`，程式退出時立即安全取消所有未執行的背景 I/O 佇列，搭配 GTK 迴圈刷新與逾時機制，杜絕退出時卡在「Waiting for jobs to finish」進度條。
* **修復 Lensfun 模組記憶體毀損與重覆解構**：修正 `plugins/lensfun/lensfun.c`，使用標準 `lf_lens_destroy()` 取代錯誤的 `lf_free()`、修復 Mounts 記憶體結構分配，並消除多執行緒暗角計算後的重覆計數解構問題。

### 13. GTK+ 3 預覽畫布背景顏色即時繪製與側邊欄目錄瀏覽區域展開修復
* **預覽背景顏色即時生效**：修正 `src/rs-preview-widget.c`，在 GTK+ 3 繪製信號處理器 (`canvas_draw_handler`) 中補齊 `preview->bgcolor` 畫布底色繪製，並修正 `rs_preview_widget_blank()` 中除以 65535 的二次重複縮放錯誤，使偏好設定 (Preferences -> General) 中的「預覽背景顏色 (Preview Background Color)」能即時且正確地反映在主畫布與邊界背景中。
* **側邊欄目錄瀏覽區域高度自適應展開**：修正 `src/gtk-interface.c` 與 `src/rs-dir-selector.c`，為右側面板「開啟 (Open)」分頁中的目錄樹狀檢視區設定垂直延展 (`vexpand = TRUE`) 與最小高度限制，解決 GTK+ 3 `GtkExpander` 容器下子視窗高度未展開、被過度壓縮至僅數十像素的顯示缺陷，使其自動延展並填滿面板可用高度。

### 14. 內建 GTK 佈景主題動態切換選單與自繪元件 Oxygen 風格深度融合
* **偏好設定內建 GTK 佈景主題動態切換**：在「偏好設定 -> 一般 (General)」中新增「GTK 佈景主題 (GTK Theme)」下拉選單，自動掃描系統已安裝之所有 GTK 3 主題（包含 `Oxygen-Mint`、`Mint-Y` 系列等）與「系統預設 (System Default)」選項。選取後立即動態套用並持久化儲存，無需重啟程式。
* **自繪元件外框與背景風格深度融合**：
  * **RGB 直方圖**：直方圖改以沉穩微漸層暗色背板渲染，RGB 三色通道與亮度（Luma）波形色彩更加清晰鮮明；外層以 `GtkFrame` (內凹邊框) 嵌合，與 Oxygen-Mint 風格完美協調；並支援在偏好設定調整高度時即時縮放。
  * **色調曲線編輯器**：加上 1px 細緻外框與 `GtkFrame` 內凹邊框，告別過去貼邊生硬的視覺感。
  * **縮圖導航膠卷與主預覽畫布**：頂部縮圖列與主預覽畫布外層啟用 `GTK_SHADOW_IN` 內凹質感邊框，將工作區與選單/工具列自然分割，呈現經典 Oxygen 沉穩、精緻的整體視覺美感。

---

## 🚀 軟體功能特性

* **直覺的 GTK+ 3 介面**：專為攝影師大量看圖、選圖與快速微調設計。
* **高精度 32 位元浮點色彩管線**：完整保留 RAW 格式高動態範圍，避免色階斷裂。
* **廣色域色彩管理**：支援 DNG Camera Profiles (`.dcp`)、ICC Profiles、sRGB、AdobeRGB 及 ProPhoto RGB。
* **多核心與向量化加速**：偵測並啟用 CPU SIMD 向量指令集（SSE2 / AVX）加速矩陣與濾鏡運算。
* **非破壞性影像調整**：曝光補償、色溫/白平衡、色調曲線、銳化、去噪、直方圖、放大鏡與分割對比預覽。
* **高效批次匯出**：非同步佇列處理，支援一鍵匯出 JPEG、TIFF 及 PNG。
* **連機拍攝 (Tethered Shooting)**：透過 `libgphoto2` 支援數位相機 USB 邊拍邊傳。

---

## 📦 安裝說明 (Linux Mint 22.3 / Ubuntu 24.04)

### 使用預編譯的 `.deb` 套件安裝

本專案根目錄下已打包產出標準 Debian 安裝檔：

```bash
cd /home/xanadu/rsync_data/geminicli/rawstudio-legacy
sudo apt install ./rawstudio-legacy_2.1-7~mint22.3_amd64.deb
```

安裝後可於應用程式選單的「**影音與圖形 (Graphics)**」類別中找到 **Rawstudio Legacy**，或直接於終端機執行：

```bash
rawstudio-legacy
# 或相容指令
rawstudio
```

### 解除安裝

```bash
sudo apt remove rawstudio-legacy
```

---

## 🛠️ 自行編譯與打包指南

### 1. 安裝建置依賴項

```bash
sudo apt update
sudo apt install -y build-essential autoconf automake libtool gettext pkg-config \
    libgtk-3-dev libglib2.0-dev liblcms2-dev liblensfun-dev libexiv2-dev \
    libgphoto2-dev libfftw3-dev libsqlite3-dev libxml2-dev libtiff-dev \
    libjpeg-turbo8-dev libdbus-1-dev dpkg-dev
```

### 2. 一鍵打包成 `.deb`

專案目錄內建已配置完成的自動化打包腳本 [`build_deb.sh`](file:///home/xanadu/rsync_data/geminicli/rawstudio-legacy/build_deb.sh)：

```bash
./build_deb.sh
```

腳本會自動完成：
1. 依賴與工具鏈檢查。
2. 多核心加速編譯所有函式庫、外掛及主程式。
3. 建立 Staging 檔案樹並建置相容符號連結。
4. 剝除除錯符號（`strip`）減少體積。
5. 調用 `dpkg-shlibdeps` 自動分析動態連結依賴（自動對齊 t64 架構）。
6. 調用 `dpkg-deb` 封裝為標準 `.deb` 檔。

### 3. 一般原始碼手動編譯與安裝

```bash
autoreconf -fiv
./configure --prefix=/usr --disable-static --enable-experimental
make -j$(nproc)
sudo make install
```

---

## 📂 檔案與設定檔目錄結構

| 資源名稱 | 系統路徑 | 說明 |
| :--- | :--- | :--- |
| **執行檔** | `/usr/bin/rawstudio-legacy` | 主執行程式（`/usr/bin/rawstudio` 為軟連結） |
| **外掛模組庫** | `/usr/lib/rawstudio-legacy/plugins/` | 30 個影像處理、解碼與匯出外掛庫 (`.so`) |
| **後端共享庫** | `/usr/lib/librawstudio-legacy-2.1.so` | 核心色彩與影像計算管線庫 |
| **全域設定與資源**| `/usr/share/rawstudio-legacy/` | 內建色調曲線、相機描述檔及 UI 定義 |
| **相機中繼資料庫**| `/usr/share/rawspeed/cameras.xml` | 現代相機感光元件定義庫 (2023 版本) |
| **桌面捷徑** | `/usr/share/applications/rawstudio-legacy.desktop` | 桌面環境啟動入口 |
| **使用者設定檔** | `~/.config/rawstudio-legacy/preferences` | GKeyFile INI 格式偏好設定 |
| **照片縮圖與快取**| `~/.rawstudio/` | SQLite 本地影像資料庫與快取 |

---

## ⚠️ 常見問題排查 (Troubleshooting)

### 執行時出現 `libgconf-2.so.4: cannot open shared object file` 錯誤

* **問題根源**：
  若先前曾透過手動原始碼編譯安裝（如 `make install`），系統的 `/usr/local/lib/` 與 `/usr/local/bin/` 殘留了早期舊版編譯的 `librawstudio-2.1.so` 與 `rawstudio` 執行檔。
  因為 Linux 動態連結器與 PATH 優先搜尋 `/usr/local/`，導致系統載入到舊版殘留檔而非新版套件庫。
* **解決方式**：
  1. 安裝最新打包的 `rawstudio-legacy`（後端函式庫已改名為獨立的 `librawstudio-legacy-2.1.so` 並內嵌 RPATH，完全隔絕舊版干擾）：
     ```bash
     sudo dpkg -i rawstudio-legacy_2.1-7~mint22.3_amd64.deb
     ```
  2. （選用）徹底清理 `/usr/local` 中已廢棄的舊版殘留檔：
     ```bash
     sudo rm -rf /usr/local/bin/rawstudio /usr/local/lib/librawstudio* /usr/local/lib/rawstudio /usr/local/share/rawstudio
     sudo ldconfig
     ```

---

## 📄 授權與宣告 (License)

* **Rawstudio Legacy** 基於 **GNU General Public License v2.0 or later (GPL-2.0-or-later)** 釋出。
* 詳細授權資訊請參閱根目錄的 [`COPYING`](file:///home/xanadu/rsync_data/geminicli/rawstudio-legacy/COPYING) 檔案。
* 現代化適配與維護倉庫：[sophAi/rawstudio-legacy](https://github.com/sophAi/rawstudio-legacy)
* 上游專案原作者：Anders Brander, Anders Kvist, Klaus Post 及開源貢獻者社群。
