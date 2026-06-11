# 《十樓的她》Hotel Victoria — UE 5.7 重建指南

> 本指南假設你使用 Windows 11、已安裝 UE 5.7（`C:\Users\<你>\Downloads\UE_5.7`）與 Visual Studio 2022。
> 目標品質對標《黑神話：悟空》的光影 / 電影感 / 材質——以 **Lumen + Nanite + Virtual Shadow Maps + Megascans** 達成。
>
> **重要聲明：本 C++ 骨架由 AI 產生、未經實際編譯驗證。** 文末「最可能需要手動調整的 5 個點」請優先閱讀。

---

## 目錄

1. [開啟專案與首次 C++ 編譯](#1-開啟專案與首次-c-編譯)
2. [Enhanced Input 資產建立](#2-enhanced-input-資產建立)
3. [角色 / GameMode 藍圖子類接線](#3-角色--gamemode-藍圖子類接線)
4. [用 Fab 免費資產快速組十層飯店](#4-用-fab-免費資產快速組十層飯店)
5. [女鬼：Mixamo 模型與動畫](#5-女鬼mixamo-模型與動畫)
6. [黑悟空風格光影](#6-黑悟空風格光影)
7. [開場 Level Sequence 逐鏡頭重建表](#7-開場-level-sequence-逐鏡頭重建表)
8. [旁白與字幕](#8-旁白與字幕)
9. [打包 Windows 與效能建議](#9-打包-windows-與效能建議)
10. [最可能需要手動調整的 5 個點](#10-最可能需要手動調整的-5-個點)

---

## 1. 開啟專案與首次 C++ 編譯

### 1.1 Visual Studio 2022 需求

開啟 **Visual Studio Installer** → 修改 VS2022，必勾：

| 工作負載 / 元件 | 說明 |
|---|---|
| **使用 C++ 的遊戲開發** | 內含「Unreal Engine 的安裝程式支援」 |
| **使用 C++ 的桌面開發** | MSVC v143 工具組 |
| Windows 11 SDK（10.0.22621 或更新） | 在「個別元件」確認 |
| .NET 8 SDK（或 VS 內附 .NET 桌面開發） | UBT/UHT 需要 |

> UE 5.7 對 MSVC 版本有最低需求；若編譯時出現 `MSVC toolchain is not supported`，到個別元件安裝較新的 `MSVC v143 - VS 2022 C++ x64/x86 建置工具`。

### 1.2 產生專案檔與編譯

1. 把本目錄（`unreal/`）整個複製到 Windows 任意路徑，**路徑避免中文與空格**，例如 `D:\Projects\HotelVictoria\`。
2. 右鍵 `HotelVictoria.uproject` → **Generate Visual Studio project files**。
   - 若右鍵選單沒有此項：執行一次 `UE_5.7\Engine\Binaries\Win64\UnrealVersionSelector.exe`（以系統管理員）註冊檔案關聯；或用命令列：
     ```
     "C:\Users\<你>\Downloads\UE_5.7\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="D:\Projects\HotelVictoria\HotelVictoria.uproject" -game -engine
     ```
3. 開啟產生的 `HotelVictoria.sln` → 上方組態選 **Development Editor / Win64** → 方案總管對 `HotelVictoria` 右鍵 → **建置**。
4. 建置成功後雙擊 `HotelVictoria.uproject` 開啟編輯器。第一次會編 Shader（Lumen + Nanite 全開，可能 20 分鐘以上），請耐心等。

> `EngineAssociation` 已設為 `"5.7"`。若你的引擎是從 zip 解壓（非 Launcher 安裝），雙擊 uproject 可能找不到引擎——請改用 UnrealVersionSelector 選取引擎路徑，或把 `EngineAssociation` 改成註冊出來的 GUID。

### 1.3 常見編譯錯誤排除

| 症狀 | 原因與解法 |
|---|---|
| `cannot open include file 'XXX.generated.h'` | UHT 還沒跑。不要直接按「僅編譯此檔」，整個專案建置；或刪 `Intermediate/` 重新 Generate project files。 |
| `unresolved external symbol` 指向 EnhancedInput / UMG 類別 | `HotelVictoria.Build.cs` 相依沒生效。確認模組名稱拼字、刪 `Intermediate/`+`Binaries/` 重建。 |
| `Plugin 'EnhancedInput' failed to load` | 引擎安裝不完整或 uproject 內外掛名拼錯。EnhancedInput 是引擎內建外掛，重新驗證引擎安裝。 |
| 編輯器開啟即崩潰，log 提到 `GlobalDefaultGameMode` | `DefaultEngine.ini` 指向的 `L_Hotel` 地圖還不存在。先建地圖（見 §4.1），或暫時把 `EditorStartupMap`/`GameDefaultMap` 兩行註解掉。 |
| `Failed to launch editor: 0xc000007b` | VS 可轉散發套件缺失。安裝 `UE_5.7\Engine\Extras\Redist\en-us\` 下的 VC redist。 |
| UHT 報 `BlueprintNativeEvent` 介面函式錯誤 | 介面實作類必須 override `XXX_Implementation`（本骨架已遵守）；自行新增函式時注意。 |
| 改了 .h 後 Live Coding 連結失敗 | Live Coding 只適合 .cpp 小改。改標頭請關編輯器、回 VS 重建。 |

---

## 2. Enhanced Input 資產建立

骨架的 `AHVPlayerCharacter` 需要 1 個 IMC + 5 個 IA 資產。

### 2.1 建立 Input Action（IA）

Content Browser → `Content/HotelVictoria/Input/` →右鍵 → **Input → Input Action**，建立 5 個：

| 資產名 | Value Type | 用途 |
|---|---|---|
| `IA_Move` | **Axis2D (Vector2D)** | 前後左右移動 |
| `IA_Look` | **Axis2D (Vector2D)** | 滑鼠視角 |
| `IA_Sprint` | **Digital (bool)** | 按住疾跑 |
| `IA_Flashlight` | **Digital (bool)** | 手電筒開關 |
| `IA_Interact` | **Digital (bool)** | 互動 |

### 2.2 建立 Input Mapping Context（IMC）

同資料夾右鍵 → **Input → Input Mapping Context** → 命名 `IMC_Default`，開啟後加入映射：

**IA_Move**（4 個鍵）：

| 按鍵 | Modifiers（依序） |
|---|---|
| `W` | Swizzle Input Axis Values（YXZ） |
| `S` | Negate → Swizzle Input Axis Values（YXZ） |
| `D` | （無） |
| `A` | Negate |

> 原理：WASD 都是 1D 輸入。Swizzle 把 W/S 的值轉到 Y 分量（前後），A/D 留在 X（左右）；C++ 端 `MoveVector.Y` 走前後、`MoveVector.X` 走左右。

**IA_Look**：

| 按鍵 | Modifiers |
|---|---|
| `Mouse XY 2D-Axis`（Mouse XY） | **Negate**：只勾 **Y**（不勾 X、Z）→ 滑鼠上推=抬頭 |

**IA_Sprint**：`Left Shift`（無 Modifier；Started/Completed 由 C++ 處理按住/放開）。
**IA_Flashlight**：`F`。
**IA_Interact**：`E`。

### 2.3 指定給角色

IMC/IA 不會自動生效——必須在 **BP_HVPlayer**（§3.1）的 Details → `HV|Input` 分類把 `DefaultMappingContext = IMC_Default`、五個 Action 一一指上。C++ 已在 `NotifyControllerChanged()` 呼叫 `AddMappingContext`，缺資產時 Output Log 會有中文警告。

---

## 3. 角色 / GameMode 藍圖子類接線

### 3.1 BP_HVPlayer

1. `Content/HotelVictoria/Blueprints/` → 右鍵 → **Blueprint Class** → 搜尋父類 `HVPlayerCharacter` → 命名 `BP_HVPlayer`。
2. Class Defaults → `HV|Input`：照 §2.3 指定 IMC 與 5 個 IA。
3. （可選）調整 `HV|Flashlight` 的強度 / 閃爍幅度、`HV|Movement` 的步行/疾跑速度。
4. 手電筒、相機、互動射線全部在 C++，藍圖不需要加任何節點即可遊玩。

### 3.2 WBP_Reading（閱讀視窗）

1. 右鍵 → **User Interface → Widget Blueprint** → 父類選 `HVReadingWidget` → 命名 `WBP_Reading`。
2. Designer 排版（建議）：
   - 全螢幕半透明黑 `Border`（Brush Color A=0.85）。
   - 中央 `Vertical Box`：`TextBlock_Title`（24pt 襯線字）、`TextBlock_Body`（自動換行）、`TextBlock_PageNum`、`Button_Next`／`Button_Close`。
3. Graph 接線（節點級）：
   - **Event OnReadingContentChanged**（C++ BlueprintImplementableEvent，在「Override」下拉找）：
     `Title` → `TextBlock_Title.SetText`；`PageText` → `TextBlock_Body.SetText`；
     `PageIndex`+1 與 `PageCount` → `Format Text ("{A} / {B}")` → `TextBlock_PageNum.SetText`。
   - `Button_Next.OnClicked` → 呼叫父類函式 **NextPage**（最後一頁會自動關閉）。
   - `Button_Close.OnClicked` → **CloseReading**。
   - 鍵盤 E/Space/Esc 已由 C++ `NativeOnKeyDown` 處理。

### 3.3 BP_HVGameMode

1. 右鍵 → Blueprint Class → 父類 `HVGameMode` → 命名 `BP_HVGameMode`。
2. Class Defaults：`ReadingWidgetClass = WBP_Reading`、`RequiredEvidenceCount = 5`、測試時 `InitialState = Play`（正式流程用 Title）。
3. **Event BeginPlay**（注入女鬼導演資產，與 ini 軟路徑擇一）：
   - `Get World Subsystem (HVGhostDirector)` →
   - → **SetEventTable**（接 `DT_GhostEvents`，見 §3.5）
   - → **SetGhostClass**（接 `BP_Ghost`，見 §5）
   - → **SetMoveEaseCurve**（接一條 0→1 的 EaseInOut `CurveFloat`）。
4. World Settings（或 Project Settings → Maps & Modes）把 GameMode 改成 `BP_HVGameMode`。

### 3.4 WBP_HUD（準心與互動提示）

1. Widget Blueprint（父類 UserWidget 即可）→ 中央小圓點 `Image`、下方 `TextBlock_Hint`（預設 Hidden）。
2. Graph：
   - **Event Construct** → `Get Game Mode` → `Cast to HVGameMode` → 對 **OnInteractHintChanged** 委派 **Bind Event**。
   - 綁定的自訂事件（參數 `bVisible`、`HintText`）：`Branch(bVisible)` → True：`TextBlock_Hint.SetText(HintText)` + `SetVisibility(Visible)`；False：`SetVisibility(Hidden)`。
   - 同法可綁 **OnEvidenceChanged** 顯示「證據 3 / 5」。
3. 關卡藍圖 BeginPlay：`Create Widget(WBP_HUD)` → `Add to Viewport`。

### 3.5 DT_GhostEvents（女鬼事件資料表）

1. 右鍵 → **Miscellaneous → Data Table** → Row Structure 選 **HVGhostEventRow** → 命名 `DT_GhostEvents`，放在 `Content/HotelVictoria/Data/`。
2. 建議起始 12 列（Row Name 隨意，欄位如下）：

| EventType | MinFloor | MaxFloor | MinEvidence | Weight | Duration | CooldownAfter | SpawnTag | bRequiresSpawnTag | bDespawnWhenSeen |
|---|---|---|---|---|---|---|---|---|---|
| WallCrawl | 2 | 9 | 0 | 1.0 | 4.0 | 50 | （空） | ✗ | ✗ |
| CeilingCrawl | 2 | 9 | 1 | 0.8 | 4.5 | 55 | （空） | ✗ | ✗ |
| ContortWalk | 3 | 9 | 2 | 0.7 | 6.0 | 70 | （空） | ✗ | ✗ |
| TVCrawlOut | 7 | 7 | 2 | 1.2 | 5.0 | 90 | `TVPoint` | ✓ | ✗ |
| ElevatorHang | 1 | 10 | 0 | **0** | 3.0 | 60 | （空） | ✗ | ✗ |
| PassBehind | 1 | 10 | 0 | 1.5 | 3.0 | 40 | （空） | ✗ | ✓ |
| StandBehind | 2 | 10 | 1 | 1.0 | 5.0 | 55 | （空） | ✗ | ✓ |
| DistantStare | 1 | 10 | 0 | 1.5 | 7.0 | 45 | `StarePoint` | ✗ | ✗ |
| DoorPeek | 2 | 9 | 0 | 1.2 | 4.0 | 45 | `DoorPeekPoint` | ✓ | ✗ |
| LightsOutFace | 3 | 9 | 3 | 0.5 | 1.6 | 110 | （空） | ✗ | ✗ |
| StairwellHang | 2 | 10 | 1 | 0.9 | 4.0 | 60 | `StairHangPoint` | ✓ | ✗ |
| LobbyCrawl | 1 | 1 | 0 | 1.0 | 7.0 | 60 | `LobbyPoint` | ✓ | ✗ |

3. **定位點擺放**：在關卡放 **TargetPoint** Actor，Details → Actor → Tags 加上對應字串（如 `DoorPeekPoint`）。導演會挑「同樓層、離玩家最近」的點。X 軸（箭頭）方向 = 女鬼移動方向（電視爬出、門縫探頭、大廳爬行都吃這個朝向）。
4. ElevatorHang 權重為 0：只能被 `AHVElevatorActor` 的一次性觸發器點名，不參與隨機。

### 3.6 證據 / 門 / 電梯擺放

- **BP_Evidence**（父類 `HVEvidenceActor`）做 5 個實例放關卡：

| 樓層 | EvidenceId | EvidenceTitle | Pages 內容提示 |
|---|---|---|---|
| 1F 辦公室 | `Evidence.Register1F` | 一樓辦公室登記簿 | 1972/10/12 之後林秀蘭的排班被人塗改 |
| 3F | `Evidence.Pawnshop3F` | 當舖存根 | 經理典當了客人「遺失」的金錶 |
| 5F | `Evidence.Confession5F` | 未寄出的懺悔信 | 同事目睹真相卻不敢作證 |
| 7F | `Evidence.Clipping7F` | 剪報 | 「女房務員畏罪墜樓」——與 TVCrawlOut 同房間 |
| 9F | `Evidence.Diary9F` | 林秀蘭的日記 | 最後一頁：「他說要帶我去十樓談」 |

- **BP_Door**（父類 `HVDoorActor`）：指定 `OpenCurve`（0→1、0.8 秒、右鍵 key → Auto 切線即 EaseOut）。`DoorMesh` 的樞紐技巧：把門板 Static Mesh 的 Relative Location 沿 Y 推半個門寬，旋轉就會繞門軸。**1013 房門**勾 `bIsRoom1013Door`（會自動初始隱形＋集滿證據浮現）。
- **BP_Elevator**（父類 `HVElevatorActor`）：指定 `DoorCurve`（0→1、1.6 秒）。`HangPoint` 預設在門楣上方 220cm，可在實例微調。踏進轎廂觸發一次性倒掛。

---

## 4. 用 Fab 免費資產快速組十層飯店

### 4.1 地圖與資料夾

1. `Content/HotelVictoria/Maps/` 建空地圖 **L_Hotel**（File → New Level → Empty Level），存檔後 `DefaultEngine.ini` 的啟動地圖即生效。
2. 基礎光照鏈：放 `Directional Light`（月光，強度 0.5 lux、色溫 9000K）、`Sky Atmosphere`、`Exponential Height Fog`（勾 **Volumetric Fog**）、`Post Process Volume`（§6.2）。室內為主，窗戶少而高。

### 4.2 Megascans（經 Fab）

1. 編輯器內 **Window → Fab**（5.7 內建 Fab 外掛）登入 Epic 帳號。
2. Megascans 整庫對 UE 免費。搜尋建議關鍵字：
   - 牆面：`damask wallpaper`、`worn plaster wall`、`art deco panel`
   - 地面：`hotel carpet`、`herringbone wood floor`（走廊紅地毯是 1970s 飯店靈魂）
   - 門窗：`wooden door vintage`、`door frame`
   - 點綴：`peeling paint decals`、`water stain decal`、`cobweb`
3. 匯入品質選 **Nanite / High**——本專案 Nanite 已全域開啟，高模直接拖進關卡無需減面。
4. **注意：City Sample（Matrix 城市資產）不適用本專案**——它是現代美式都市建築與車輛，風格、室內結構都不合 1972 歐式飯店，且體積巨大（>100GB），不要下載。

### 4.3 標準層：Level Instance / Packed Level Actor

十層樓中 2F–9F 結構相同，務必做「標準層」：

1. 先在 L_Hotel 用 Megascans 模組搭好**一層**：電梯廳 + T 字走廊 + 6~8 間客房門 + 樓梯間。層高建議 **500cm**（與 `DefaultGame.ini` 的 `FloorHeight=500` 一致；若你改層高，記得同步改 ini）。
2. 全選該層所有 Actor → 右鍵 → **Level → Create Packed Level Actor**（純靜態網格用 Packed，含 BP_Door 等邏輯 Actor 則選 **Create Level Instance**）→ 存成 `LI_TypicalFloor`。
3. 複製 8 份，Z 依序 +500、+1000…堆出 2F–9F。改一處、處處生效。
4. 1F（大廳挑高、辦公室、旋轉門）與 10F（走廊盡頭的 1013 房）單獨手搭。
5. 每層樓電梯廳放 `BP_Elevator` 或假電梯門；樓梯間放 `StairHangPoint` 定位點；走廊兩端放 `StarePoint`。
6. 導航：放一個 **NavMeshBoundsVolume** 罩住全樓（女鬼目前用參數化位移不依賴 NavMesh，但留著供日後 AIModule 巡走擴充）。

---

## 5. 女鬼：Mixamo 模型與動畫

### 5.1 下載

1. [mixamo.com](https://www.mixamo.com)（Adobe 免費帳號）→ Characters 選白衣長髮類角色（如 **Claire**）。
2. 下載角色：FBX Binary、**T-Pose**。
3. 動畫建議清單（Download 時選 FBX、**Without Skin**、30fps）：
   - `Crawling`（地面爬行 → CrawlFloor / LobbyCrawl / TVCrawlOut）
   - `Zombie Crawl`（變化用）
   - `Walking`（PassBehind）
   - `Idle` / `Standing Idle`（StandBehind / DistantStare）
   - `Hanging Idle`（倒吊基底 → ElevatorHang / StairwellHang）
   - `Crouched Walking`（反折行走基底）

### 5.2 匯入與 IK Retargeter

1. 匯入角色 FBX：Skeletal Mesh 勾選、Material 匯入後换成溶解材質（§5.4）。
2. 匯入動畫 FBX：**Skeleton 指向剛才角色的 Skeleton**（同樣來自 Mixamo 的動畫多半可直接共用骨架，免 Retarget）。
3. 若骨架不相容（動畫一團亂）才需 Retarget：
   - 對來源與目標 Skeletal Mesh 各建 **IK Rig**（右鍵 → Animation → Retargeting → IK Rig）：設 Retarget Root = `Hips`，加 Chain：Spine（spine→spine2）、Head（neck→head）、左右 Arm（shoulder→hand）、左右 Leg（upLeg→foot）。
   - 建 **IK Retargeter**：Source = 來源 IK Rig，Target = 目標 IK Rig，開啟後在 Asset Browser 選取動畫 → **Export Selected Animations**。
4. Mixamo 模型匯入後若朝向不對：在 BP_Ghost 把 Mesh 元件 Yaw 設 -90（UE 角色慣例面向 +X）。

### 5.3 Control Rig 調姿態（爬行 / 倒吊）

Mixamo 沒有「天花板爬行」「反折」這種動畫，用 Control Rig 在現有動畫上疊修：

1. 對 Skeletal Mesh 右鍵 → Create → **Control Rig**。Rig Graph 中 Forward Solve 給 `Hips`、四肢 IK 加 Control。
2. **天花板爬行**：C++ 已將 Mesh Roll 翻轉 180 度作佔位；進階做法是在 AnimBP 的 CrawlCeiling 分支播 `Crawling` 並用 Control Rig 節點把脊椎反弓、頭部 Aim 朝下（朝玩家）。
3. **倒吊**：以 `Hanging Idle` 為底，在 Anim Sequence 上 Create → Create Animation → Current Pose 後用 Control Rig 把雙臂下垂、頭後仰 30 度，烘焙成新動畫 `A_HangInverted`。
4. **反折行走**：`Crouched Walking` 為底，Control Rig 把 Spine 各節 Pitch 反折 -25 度 × 3 節、頭 Twist 170 度，烘焙 `A_ContortWalk`。寧可僵硬，不要平滑——僵硬才恐怖。

### 5.4 AnimBP 與溶解材質

1. **ABP_Ghost**：Animation Blueprint，父類選 **HVGhostAnimInstance**，Skeleton 選 Mixamo 角色骨架。AnimGraph 建 State Machine，以 `GhostMode`（enum，C++ 每幀更新）分支：`Stand→Idle`、`Walk→Walking`、`CrawlFloor/CrawlWall/CrawlCeiling→Crawling`、`HangInverted→A_HangInverted`、`ContortWalk→A_ContortWalk`、`DoorPeek→Idle(上半身傾斜 Layered Blend)`、`FaceClose→Idle(快門快開)`、`Hidden→Idle`。
2. **溶解材質 M_GhostDissolve**：
   - Blend Mode = **Masked**，雙面。
   - `Noise Texture (Perlin)` → 與 Scalar Parameter **`Dissolve`**（0~1，名稱必須與 C++ `DissolveParamName` 一致）比較：`If (Noise - Dissolve > 0)` → Opacity Mask。
   - 溶解邊緣：`Dissolve` 加 0.05 再比較一次，兩者相減的窄帶 × 冷青色（0.3, 0.9, 1.0）× 20 → **Emissive**，得到發光蝕邊。
   - Base Color 壓暗（×0.15）、Emissive 微弱青光——女鬼主要靠手電筒照出。
3. **BP_Ghost**：Blueprint 父類 `HVGhostCharacter`，Mesh 指 Mixamo 模型、Anim Class 指 ABP_Ghost、材質全槽换 M_GhostDissolve。C++ 會自動為所有材質槽建 MID 並推 `Dissolve` 參數，淡入淡出即溶解顯隱。
4. 回 §3.3 把 `BP_Ghost` 注入 GhostDirector。

---

## 6. 黑悟空風格光影

核心公式：**全動態光照（Lumen）＋極少光源＋體積霧＋Filmic 後製**。骨架的 `DefaultEngine.ini` 已開：Lumen GI/反射、VSM、Nanite、TSR、體積霧、Film Grain。

### 6.1 Lumen 細調（Post Process Volume 內）

| 設定 | 值 | 理由 |
|---|---|---|
| Global Illumination → Lumen Scene Detail | 2.0 | 走廊小物件也參與 GI |
| Lumen Scene Lighting Quality | 2.0 | 反彈光乾淨 |
| Final Gather Quality | 2.0~4.0 | 暗部噪點壓制（高=費效能） |
| Max Trace Distance | 5000 | 室內夠用，省效能 |
| Reflections → Quality | 2.0 | 大理石地板、鏡子 |
| 主控台 `r.Lumen.Reflections.ScreenSpaceReconstruction 1` | — | 反射降噪 |

有 RTX 顯卡可在 Project Settings → Rendering 開 **Hardware Ray Tracing + Lumen HWRT**（`r.Lumen.HardwareRayTracing=1`），鏡面與細陰影更接近黑悟空；GTX 級顯卡維持軟體 Lumen 即可。

### 6.2 Post Process Volume（電影感）

放一個 PPV、勾 **Infinite Extent**：

| 區塊 | 參數 | 建議值 |
|---|---|---|
| Exposure | Metering Mode | Auto Exposure Histogram |
| | Min EV100 / Max EV100 | **-2.0 / 1.0**（鎖死曝光範圍，走廊永遠暗） |
| | Exposure Compensation | -0.5 |
| Color Grading | Temp | 4500K（鎢絲燈昏黃） |
| | Saturation | 0.85（褪色 70 年代） |
| | Shadows Gain | 0.9、Toe 提一點保留死黑 |
| Film | （UE5 預設即 Filmic/ACES Tonemapper） | Slope 0.5 / Toe 0.6 微調對比 |
| Bloom | Method = Convolution、Intensity | 0.4（光暈收斂才高級） |
| Vignette | Intensity | 0.55 |
| Film Grain | Intensity | 0.35（1972 膠片感的關鍵） |
| Chromatic Aberration | Intensity | 0.15（鏡頭邊緣輕微色散） |
| Motion Blur | Amount | 0.35、Max 5% |
| Lens Flare | Intensity | 0（恐怖片不要花） |

### 6.3 體積霧＋手電筒

- `Exponential Height Fog`：Fog Density 0.03、勾 **Volumetric Fog**、Scattering Distribution 0.6、Albedo 灰白。
- 手電筒（C++ SpotLight）已設 `VolumetricScatteringIntensity = 2.0` → 光錐在霧中實體化，掃過走廊時灰塵感即出。
- 走廊壁燈也各給 Volumetric Scattering 0.5，燈下會有小光球。

### 6.4 走廊壁燈 Flicker（藍圖 Timeline）

1. 建 `BP_WallLamp`：Static Mesh（燈座）+ Point Light（強度 8 cd、色溫 2700K、Attenuation 600、**Cast Shadow 關**——一層十幾盞全開陰影會炸效能，只給電梯廳主燈開陰影）。
2. Event Graph：
   - **BeginPlay** → `Retriggerable Delay (Random Float in Range 0~5)` → `Timeline_Flicker.Play from Start`（錯開各盞相位）。
   - **Timeline_Flicker**：長度 4 秒、勾 Loop。加 Float 軌 `Brightness`：key 大致 `(0, 1.0) (0.3, 0.92) (0.35, 1.0) (1.7, 1.0) (1.75, 0.4) (1.82, 1.0) (2.6, 0.85) (4.0, 1.0)`——平穩中突然抽一下。
   - Update → `Brightness × 8.0` → `Point Light.Set Intensity`。
3. **熄燈貼臉聯動**：關卡藍圖 BeginPlay → `Get World Subsystem (HVGhostDirector)` → Bind Event to **OnLightsOutRequested** → 自訂事件（參數 `bLightsOut`）→ `Get All Actors Of Class (BP_WallLamp)` → ForEach → `Point Light.SetVisibility(NOT bLightsOut)`。女鬼導演熄燈事件結束會自動廣播復燈。

---

## 7. 開場 Level Sequence 逐鏡頭重建表

`Content/HotelVictoria/Cinematics/` → 右鍵 → Cinematics → **Level Sequence** → `LS_Opening`。每鏡頭一台 **Cine Camera Actor**，用 **Camera Cut Track** 切換。總長約 75 秒 @30fps。

| # | 時間 | 鏡頭 | 內容與 Key | 備註 |
|---|---|---|---|---|
| 1 | 0:00–0:08 | 遠景 | 相機距飯店 40m、focal 35mm，緩慢 Dolly In（位置 X 兩 key 推 6m），仰角 8 度看十層立面 | 雨夜，閃電（Directional Light 強度脈衝 key）打亮「HOTEL VICTORIA」霓虹殘字 |
| 2 | 0:08–0:16 | 大門 | focal 50mm，正對銅框旋轉門，Dolly In 3m 至門前 | 門縫漏出大廳微光；蛛網 decal |
| 3 | 0:16–0:28 | 大廳 | 進門後 Pan 左→右 60 度（Rotation Yaw 兩 key），掃過接待櫃台、吊燈、停擺座鐘 | 座鐘永遠停在 2:47——林秀蘭墜樓時刻 |
| 4 | 0:28–0:36 | 走進電梯 | 低機位（Z=120）跟拍背影視角前推 5m 進電梯廳，電梯門已開著等 | `BP_Elevator.SetDoorsOpen(true)` 由 Sequence 的 Event Track 呼叫 |
| 5 | 0:36–0:50 | 電梯上升 | 相機固定轎廂內後角；Event Track 關門；樓層指示燈材質參數 1→10 的 Float Track | 加 **轎廂微震**：相機 Transform 加噪聲——Camera Shake 見下方 |
| 6 | 0:50–0:58 | 十樓開門 | 門開（Event Track），走廊盡頭站著她（BP_Ghost 擺進 Sequence、Spawnable）；她以 Transform Track 從 12m 處「滑」近至 1m（不走路，整體平移更瘆人），頭髮遮臉 | 同時 Fog Density Track 0.03→0.12 |
| 7 | 0:58–1:04 | 墜落 | 鏡頭猛然上仰→Roll 翻轉，Transform Z 從 +4500 直落 0（key 兩端，中段 Ease In 加速度感）；畫面邊緣加運動模糊 | **墜落震動曲線**見下 |
| 8 | 1:04–1:15 | 標題 | 黑 1 秒 → Fade Track 開：仰視天花板視角（她墜地者的視角），淡入標題「十樓的她」字卡（UMG 或 Sequence 文字） | 心跳聲 ×2 收尾 |

**墜落震動（Camera Shake）**：
1. 右鍵 → Blueprint Class → **LegacyCameraShake**（或 PerlinNoiseCameraShakePattern 的 CameraShakeBase）→ `CS_Fall`：Rot Pitch Amplitude 4 / Frequency 18，Loc Z Amplitude 8 / Frequency 22，Duration 6。
2. Sequence 在鏡頭 7 加 **Camera Shake Track**（對該 Camera Cut 的相機）指向 `CS_Fall`；落地瞬間再疊一個單發大振幅 `CS_Impact`（Pitch 12、Duration 0.3）。
3. 電梯段（鏡頭 5）用小振幅版 `CS_ElevatorRumble`（Amplitude 0.6 / Frequency 7、Duration 14）。

**接回遊戲**：Sequence 結尾加 Event Track → 綁定關卡藍圖事件 → `Get Game Mode → Cast to HVGameMode → StartPlay1972`。關卡藍圖 BeginPlay 判斷 `GetGameState == Title` 時 `Create Level Sequence Player(LS_Opening)` → `Play`，並先呼叫 `SetGameState(Cinematic)`。

---

## 8. 旁白與字幕

### 8.1 取音

- **免費 TTS**：Edge TTS（`zh-TW-HsiaoChenNeural` 沙啞調低語速）或 Coqui/Bark 本地生成；輸出 48kHz WAV。商用前確認授權。
- **自錄**（建議）：手機 + 棉被吸音即可。旁白是調查者的內心獨白，氣音、貼麥。
- 命名規範：`VO_Op_01.wav`、`VO_Ev_Register1F.wav`…匯入 `Content/HotelVictoria/Audio/VO/`。

### 8.2 MetaSound

1. 右鍵 → Audio → **MetaSound Source** → `MS_VO_Player`。
2. Graph：`Wave Player` 節點（Wave Asset 升級成 **Input** 參數，名 `VOWave`）→ 串 `Stereo Delay`（Dry 0.9 / Wet 0.1，廢墟微殘響）→ Output。`On Finished` 接 MetaSound 的 On Finished 輸出。
3. 播放：藍圖 `Spawn Sound 2D (MS_VO_Player)` → `Set Wave Parameter("VOWave", VO_xxx)`。包一個 `BP_VOManager`（Actor，放關卡）統一管佇列，避免旁白互疊。

### 8.3 字幕 UMG

1. `WBP_Subtitle`：底部置中 `TextBlock`（白字、黑色 Outline 2px、80% 螢幕寬自動換行）。
2. `BP_VOManager` 維護 `Map<SoundWave, FText>`（或一張 DataTable：`VOKey, Wave, SubtitleText, Duration`）。播放 VO 時同步 `WBP_Subtitle.SetText` 顯示，`On Finished`（或 Duration 計時）後隱藏。
3. 觸發點：撿證據後（綁 GameMode `OnEvidenceChanged`）、進入新樓層（Box Trigger）、開場 Sequence 的 Event Track。

---

## 9. 打包 Windows 與效能建議

### 9.1 打包

1. Project Settings → Packaging：`Build Configuration = Shipping`、`Use IoStore` 與 `Compressed` 已在 ini 開啟；List of maps to cook 確認含 `L_Hotel`。
2. Platforms → Windows → **Package Project** → 選輸出資料夾。
3. 常見打包失敗：
   - `Missing precompiled manifest for 'EnhancedInput'` → 用 Launcher 引擎正常；自編引擎需 `-precompile`。
   - Cook 卡在某資產 → Output Log 找 `LogCook: Error`，多半是壞引用，用 Reference Viewer 清。
   - Shipping 找不到 ini 自訂區段 → `[/Script/HotelVictoria.HVGhostDirector]` 屬 Config=Game，會打進包，不用搬。

### 9.2 效能建議（1080p / 60fps 目標，RTX 3060 級）

| 項目 | 建議 |
|---|---|
| Lumen | 軟體 Lumen + `r.Lumen.ScreenProbeGather.RadianceCache 1`；Final Gather Quality ≤ 2 |
| VSM | 投影陰影燈控制在每層 2~3 盞；壁燈關陰影（§6.4） |
| Nanite | 牆地天花全 Nanite；半透明與 Masked 材質物件不吃 Nanite，蛛網等改薄片 LOD |
| TSR | `r.ScreenPercentage 67`（1080p 內部渲染 720p，TSR 拉回）幾乎無損 |
| 體積霧 | `r.VolumetricFog.GridPixelSize 16`（預設 8 太貴） |
| 標準層 | Level Instance 開 **World Partition / Level Streaming**，非當前樓層卸載；至少對每層做 Distance-based 流送 |
| 女鬼 | 同時僅 1 隻（導演已保證）；SkeletalMesh 關 `Update Rate Optimization` 不需要，但關掉 Cast Shadow（C++ 已關） |
| 量測 | `stat unit`、`stat gpu`、`r.Nanite.Visualize Overdraw` 找熱點 |

---

## 10. 最可能需要手動調整的 5 個點

> **再次強調：本骨架未經 UE 5.7 實際編譯驗證。** 以下依風險排序，編譯失敗請先查這五處：

1. **`UHVGhostDirector` 的 Tickable Subsystem 介面**（`HVGhostDirector.h/.cpp`）
   `UTickableWorldSubsystem` 在不同引擎版本對 `Tick / GetStatId / DoesSupportWorldType` 的簽名與純虛要求略有變動。若報「abstract class」或 override 不匹配，對照 5.7 的 `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h` 修正簽名；`RETURN_QUICK_DECLARE_CYCLE_STAT` 需要 `#include "Stats/Stats.h"`（多數情況 CoreMinimal 已帶入，報錯就補）。

2. **介面 `Execute_` 呼叫與 `_Implementation` 簽名**（`HVInteractable.h`、各互動 Actor）
   `BlueprintNativeEvent` 介面在 UHT 的規則嚴格：`Execute_CanInteract(Target)` 要求 Target 為 `UObject*` 且實作類正確 override `CanInteract_Implementation()`。若 UHT 報錯，檢查 `AHVDoorActor` 等類的 override 是否與介面宣告完全一致（含參數、無 const）。

3. **Enhanced Input 標頭與類型**（`HVPlayerCharacter.cpp`）
   5.7 中 `EnhancedInputSubsystems.h / EnhancedInputComponent.h / InputMappingContext.h` 的路徑與 `ETriggerEvent` 列舉應如本骨架；若引擎調整過 include 路徑（曾發生於 5.1），依編譯錯誤把 include 改為 `EnhancedInput/Public/...` 對應新路徑即可。

4. **`UHVReadingWidget::SetIsFocusable` 與 `NativeOnKeyDown`**（`HVReadingWidget.cpp`）
   `SetIsFocusable()` 於 5.2+ 取代直接寫 `bIsFocusable`；若 5.7 再改 API（或在 `NativeOnInitialized` 時機過早），改到 `NativeConstruct` 呼叫，或在 WBP 的 Details 勾 `Is Focusable`。鍵盤事件吃不到時，確認 GameMode 設定 `FInputModeUIOnly::SetWidgetToFocus` 已生效。

5. **`DefaultEngine.ini` 的啟動地圖與渲染 CVar**
   `L_Hotel` 地圖建立前，編輯器/打包會因找不到 `GameDefaultMap` 噴警告甚至開不進關卡——先建地圖或暫時註解該三行。另外個別 CVar（如 `r.Lumen.ScreenTracingSource`、`r.FilmGrain`）在 5.7 可能更名；啟動 log 出現 `Unknown console variable` 時刪掉該行，改在 PPV 中以同名功能設定。

---

### 建議的內容資料夾結構（供對照）

```
Content/HotelVictoria/
├── Maps/            L_Hotel
├── Blueprints/      BP_HVGameMode, BP_HVPlayer, BP_Ghost, BP_Door, BP_Elevator,
│                    BP_Evidence_*, BP_WallLamp, BP_VOManager
├── Input/           IMC_Default, IA_Move, IA_Look, IA_Sprint, IA_Flashlight, IA_Interact
├── UI/              WBP_HUD, WBP_Reading, WBP_Subtitle
├── Data/            DT_GhostEvents, Curve_DoorOpen, Curve_ElevatorDoor, Curve_GhostEase
├── Characters/Ghost/ Mixamo 模型、動畫、ABP_Ghost、M_GhostDissolve
├── Cinematics/      LS_Opening, CS_Fall, CS_Impact, CS_ElevatorRumble
├── Audio/VO/        旁白 WAV 與 MS_VO_Player
└── Megascans/       Fab 匯入資產（自動建立）
```

祝重建順利。記住恐怖遊戲的鐵律：**你看不見的，永遠比看得見的可怕**——女鬼出場永遠吝嗇一點。
