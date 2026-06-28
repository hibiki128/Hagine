# GPUパーティクル(CSParticle) 最適化メモ

> GPUパーティクルを「もっと軽く・大量に」出すための最適化の作業ログ兼ロードマップ。
> 別セッション / 引き継ぎ用。最終更新: 2026-06-28（**§8 生存リスト間接ディスパッチ完了で一旦区切り**。Draw の overdraw 対策まで実装。詳細は下の「総括」）

---

## ★総括（2026-06-28・パーティクル最適化を一旦終了）

### これまでの成果（時系列・実機確認済み）
1. **SoA 化**（156B 単一 AoS → 機能別6バッファ）: 非コアレス解消。300万 Update **25→10ms**。
2. **color RGBA8 pack / scale half pack**: DrawCore/SimCore 縮小。450万 **10→7.4ms**。これ以降は帯域律速を卒業。
3. **描画コンパクション**（`gRenderCompact` 順次列）: 描画 VS の散乱 gather を排除。
4. **演出なし軽量 Update バリアント + ブロック256化**: レジスタ削減→占有率↑。4.125M **8→3.8ms**（SoA前比 約1/6）。
5. **Trail/Rotation/Override の条件付き確保**: 演出なし 148→96B/体。35M 作成可に。
6. **★§8 生存リスト間接ディスパッチ（今回の本命・完了）**: Update を「全 MAX スロット走査」→「生存リストだけ処理」に変え **O(生存数)** 化。MAX を大きくしても**疎なら Update ~0.1ms 近辺**。§1.5 の「dispatch量律速（MAX固定コスト）」を根本解決。Step1(ping-pong基盤)→Step2(Emit append+Update を listIn 入力)→Step3(dispatch本数を in リスト長由来)を実機1ステップずつ確認しながら投入。
7. **Draw overdraw 対策（距離カリング＋画面サイズ上限＋微小カリング）**: 実装済・デフォルトOFF・プレビューでも反映。**ただし当方のテストシーンでは Draw ms は大きく変わらず**＝そのシーンは fillrate/overdraw 律速ではなかった（＝道具は用意したが効くのは半透明が大量に重なるシーンのみ）。

### 現状の状態
- **律速の変遷**: 帯域(SoA/pack) → 占有率(軽量化/256) → dispatch量(§8で解消) → 次は要計測。
- **Update は疎で激安**になった（§8）。**Draw は当シーンでは支配的でなかった**（overdraw 対策の効果が薄かったのはこのため）。
- VRAM: 演出なし 96B/体 + §8 ping-pong で aliveList が ×2（+約4B/体）= 約100B/体。RTX3060 Laptop(6GB) 実用枠約4GBで演出なし約40M が目安（1億は16GB級GPUが要る）。
- **全実装ビルドOK・未コミット**（feature/player 作業ツリー）。コミットはユーザー指示待ち。

### ★次にやる予定（再開時の優先順）
> まず **全パス計測**（Emit/Update/Draw/GPU合計を 4.125M / 35M で `GpuProfiler` 計測）で**今の律速を確定**してから着手する（§8 で律速が変わったので当てずっぽうで進めない）。
1. **計測でボトルネック確定**（最優先・道具は揃っている。Update が疎で激安になった今、Emit か Draw か GPU 同期かを実測で見極める）。
2. **メモリ削減で「もっと出す」**: Emit が演出なし group でも常時書く Trail/Rotation/Override を**ゲート**してダミーへの無駄書きを排除 / 省メモリ描画モード（速度↔メモリのトレードオフ）。VRAM が「大量化」の上限なので効く。
3. **Emit の軽量バリアント化**（演出なし group 用。Update と同じ発想でレジスタ削減）。
4. **lite 版ローカル `Particle` のスリム構造体化**（必要フィールドだけ＝VGPR削減→占有率↑）。
5. **Draw をさらに**（当シーンでは効果薄だったが、半透明大量シーン向け）: CS描画 / quad展開 / タイル分割（`gRenderCompact` 順次列の土台を活用）。overdraw 対策は実装済みなので必要なシーンで ON にして検証。
6. 対抗馬（優先度低）: ダブルバッファ（Compute(N)/Draw(N-1) オーバーラップ）= VRAM2倍で「もっと出す」と逆行。

> 詳細な各候補は §6.5 ロードマップ、完了済みの §8 仕様、診断は §1.5 を参照。

---

## 0. 目的とアプローチ

- ゴール: GPUパーティクルをできる限り軽くし、より多く出す。
- 方針: **まず実測 → 律速箇所を1つずつ潰す**。VRAMを増やす施策（ダブルバッファ等）は「もっと出したい」と逆行するので後回し。
- ビルド = コンパイル検証の主手段（自動テスト無し）。**HLSLは実機起動時にDXCコンパイル**されるため、シェーダのエラーはビルドに出ず**実機起動時にassert**する点に注意。構造体のバイトずれが一番危ない。

### ビルド
```
MSBuild Hagine.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal -nologo
```
MSBuild: `C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe`
出力: `..\Generated\Outputs\$(Configuration)\Hagine.exe`。`dinput.h DIRECTINPUT_VERSION` 警告は無害。

### GPU計測
`GpuProfiler`（`Engine/Utility/Debug/GpuProfiler/`）。統計窓「GPUプロファイラ(パス別)」で Emit/Update/Draw を GPUタイムスタンプ計測（3F遅延）。FPSは60固定キャップ(`UpdateFixFPS`)なのでFPSでは余力が見えない → **必ずGPU msで見る**。

---

## 1. 計測の推移（成果サマリ・★は実機実測値）

| 段階 | 体数 | Update | 備考 |
|---|---|---|---|
| 最適化前 | 300万 | **25ms** | 156B丸ごと load→store=312B/体。帯域律速 |
| ① SoA(6本分割) | 450万 | **10ms** ★ | 非コアレス解消。1体あたり約3.7倍速 |
| ② color RGBA8 pack | 450万 | **7.4ms** ★ | DrawCore 52→40B |
| ③ 描画コンパクション | 450万 | 7.4→8〜10 | Update微増/Drawへ移譲(5→2ms見込)。④pack込み4.125Mで**8〜10ms**★ |
| ④ scale/initialScale half pack | 4.125M | 8〜10ms ★ | pack効果小=もう帯域律速でない(下記診断) |
| ⑤ 演出なし軽量Updateバリアント | 4.125M | **6.8〜7.1ms** ★ | レジスタ削減→占有率↑。帯域同一でUpdate減=占有率律速の確証 |
| ⑥ 軽量版ブロック256化 | 4.125M | **3.8〜4.0ms** ★ | 1024は占有率67%頭打ち→256で常駐1536達成。SoA前25ms比**約1/6** |
| ⑦ 条件付き確保(Trail/Rot/Override) | 35M作成可 | — | 148→96B/体。35M作成成功(従来は不可) |
| ⑧ 生存コンパクションのグループ単位atomic集約 | 35M | 疎18〜20/密100超 ★ | atomic競合は主因でなかった(下記§1.5) |

- 律速の変遷: **帯域(SoA/pack)→ 占有率(軽量化/256)→ いまは「dispatch量律速」（§1.5）**。
- ①〜⑧実装済み・**ビルドOK**・**全て未コミット**（feature/player 作業ツリー）。

> ### ★現状診断（2026-06-27, ④実機計測後）— **帯域律速は卒業した**
> 機種=**RTX 3060 Laptop**（GDDR6 192bit ≈ 300+ GB/s理論）。④実機=**4,125,000体で Update 8〜10ms**（演出なし構成）。
> 実効帯域 = 4.125M×144B(load52+store52+compaction40) ≈ 594MB / 8ms ≈ **74 GB/s** = 理論の**2割強しか出ていない**。
> → **もう帯域律速ではなく「レイテンシ/オキュパンシ律速」**。④(pack)で体感が変わらなかったのはこのため（想定通り）。
> 25→7.4ms の勝ちは SoA が非コアレス(37→80GB/s)を直した分であり、ここから先は**バイト削減では動かない**。
> **最有力仮説**: Update が単一巨大カーネルで全演出コード(curl/vortex/field/trail)を抱え**レジスタ過多→低オキュパンシ→DRAMレイテンシを隠せない**。
> **次の一手** → §6 の優先度を改訂（**演出なし専用の軽量Updateバリアント**が本命）。まず全パス計測で③のDraw効果(5→2ms?)を確定する。

---

## 1.5. ★最新診断（2026-06-28, ⑤⑥⑦⑧実機計測後）— **いまは「dispatch量律速」**

⑤⑥で 4.125M を **3.8〜4.0ms** まで短縮（占有率律速を解消）。次に⑦で 35M 作成可能になったが、**MAX=35M の Update が**:
- **生存10万（疎）でも 18〜20ms**
- **大量生存（密）で 100ms 超**

**確定事実**: Update は毎フレーム **maxParticleCount 個ぶんのスレッドを起動**して全スロットの `gLife` を走査する。よって **Update コストは「生存数」ではなく「MAX(バッファ全体)」に比例**する。MAX=35M を置いた時点で、生存が少なくても毎フレーム3500万スレッド起動の固定コスト（18〜20ms）が乗る。

**⑧（atomic集約）は的外れだった**: 密の100ms超を解消できず＝**単一カウンタへのatomic競合は主因ではなかった**。むしろ早期returnを撤廃しバリア化したぶん、疎で全スロットが同期コストを払い逆効果の懸念（要切り分け）。

→ **本質的な解は唯一「生存粒子だけ処理する」＝生存リスト間接ディスパッチ**。Update を **O(生存数)** にすればMAXをいくら大きくしても疎なら ~0.1ms。**完全仕様を §8 に記載**（次セッションで実機反復しながら実装するのが安全。横断改修＋HLSL実機コンパイルのためノーテスト一括投入は起動assertリスク大）。

---

## 2. 実装済みの最適化

### ① SoA（Structure of Arrays）分割 ★完了・実機検証済(効果確認)

旧 `outputParticleResource_`（156B単一AoS, `CSParticle`）を**機能別6バッファ**に分割。
Update CS は「使う機能のバッファだけ load/store」して帯域を削る。粒度は「細かめ(6本)」。

| バッファ | 型/サイズ | 内容 | load/store条件 |
|---|---|---|---|
| `gLife` | float 4B | lifeTime | 常時（**最初に判定→死亡/未使用slotは4Bでreturn**） |
| `gDrawCore` | `PDrawCore` 40B | translate, scale, velocity, color(RGBA8) | 常時 |
| `gSimCore` | `PSimCore` 20B | currentTime, initialScale, isTrailParticle | 常時 |
| `gTrail` | `PTrail` 20B | parentIndex, lastTrailPosition, trailSpawnDistance | `enableTrail \|\| fieldCount>0` のみ |
| `gRotation` | `PRotation` 24B | rotation, angularVelocity | `enableRandomRotation \|\| enableRandomAngularVelocity` のみ |
| `gOverride` | uint2 8B | settingsOverrideFlags(lo/hi) | `fieldCount>0` のみ |

- **endScale 廃止**: 旧 per-particle endScale は Emit で `gSettings.endScaleValue` を入れていただけで不変 → Update で設定値を直読み（12B削減）。
- Update本体ロジックは**ローカル `Particle p`（レジスタscratch）に load→従来通り処理→scatter store** で完全温存。機能ゲートは `useTrail/useRotation/useOverride`（main冒頭で判定）。
- **早期return**: `gLife[i]` だけ読み `lifeTime<=0` で離脱（死亡/未使用slotは他5本に触れない）。

### ② color を RGBA8 pack ★完了・実機検証済(効果確認)

`DrawCore.color` を float4(16B) → **RGBA8 unorm pack(uint 4B)**。DrawCore 52B→**40B**。
- HLSL `PackColorRGBA8` / `UnpackColorRGBA8`（`Particle.hlsli`）で**バッファ境界のみ**変換。計算は float4 のまま温存。
- 範囲は `saturate` で [0,1] クランプ。**注意**: フィールドの colorMultiply で 1.0 超の設定をしていた場合、旧float4は超過値をそのまま加算ブレンドできたが、今は1.0頭打ち（HDR的に光らせていた箇所は少し暗くなりうる）。粒子用途では8bitで十分。

### ④ scale / initialScale を half3 pack ★実装済・ビルドOK・実機検証待ち

§6.A を実施。Update の常時loadバッファ(DrawCore/SimCore)の scale 系を fp16 化して帯域を削る。
全CSは `cs_6_0` のため 16bit型は使わず **`f32tof16`/`f16tof32` 手動pack**（uintに詰める。プロジェクト全体の
シェーダモデル引き上げを回避）。pack/unpack は**バッファ境界のみ**、ローカル `Particle p` は float3 のままなので
物理ロジックは無改変。`translate`/`velocity` は fp16 化しない（位置精度・速度積分のドリフト/ジッタ回避）。

| バッファ | 旧 | 新 | 内訳 |
|---|---|---|---|
| `gDrawCore` / `gRenderCompact` | 40B | **36B** | translate12 + scaleXY(uint)4 + scaleZ(uint)4 + velocity12 + color4 |
| `gSimCore` | 20B | **12B** | currentTime4 + initialScaleXY4 + initialScaleZ_isTrail4 |

- scale(float3)→ `scaleXY`(=half x \| half y<<16) + `scaleZ`(=half z, 上位16bit空き) の2 word。
- **isTrailParticle(0/1) は SimCore の `initialScaleZ_isTrail` 上位16bitに同梱**（空き領域を無駄なく活用→SimCore 8B減）。
- HLSLヘルパ: `PackScaleXY` / `PackScaleZ` / `UnpackScale3(xy,z)` / `PackScaleZTrail(s,isTrail)`（`Particle.hlsli`）。
  `UnpackScale3` は z word の下位16bitのみ参照するので DrawCore/SimCore 双方で共用可。
- 常時loadパス: Life4 + DrawCore36 + SimCore12 = **52B**（旧64B, -12B ≈ -19%）。`gRenderCompact` も36B化で描画帯域も微減。
- fp16精度: scale典型値0.1〜5で相対誤差~0.05%、視覚的に無視可能。
- 改修: `ParticleStruct.h`(構造体+static_assert 36/12) / `Particle.hlsli`(構造体+helper) /
  `EmitParticle`(scale書込pack) / `UpdateParticle`(load/store/trail-spawn pack) / `ParticleCS.VS`(scale unpack)。

### ③ 描画コンパクション ★実装済・ビルドOK・実機検証待ち

旧描画VSは `instanceId → gAliveList[i] → slot → gDrawCore[slot]` の**散乱gather**で、これがDraw 5msのほぼ全部（180MBを実効~36GB/s）。
→ Updateの生存コンパクション時に **`gRenderCompact`(u11)** へ詰めた順(instanceId順)で描画データ(DrawCore=40B, 既存ローカル`odc`流用)を書き出し、描画VSは `gRenderCompact[instanceId]` を**順次読み**(t0差し替え)。
- 回転だけ従来通り `gRotation[gAliveList[instanceId]]`（回転グループのみscatter）。
- `gDrawCore` は sim専用化（VS用SRV廃止）。
- **整合性**: VSは `instanceId >= gAliveCount[0]` を先に判定してから `gRenderCompact[instanceId]` を読むので未書込み領域は読まない。`gAliveCounter`(u10) と `gAliveCount`(t3) は**同一リソース**（Updateが詰めた数 == VSがカリングに使う数）。drawCountは旧`aliveDrawCount`+marginで発行（既存挙動・1〜2F遅延許容）。

---

## 3. アーキテクチャ詳細

### 構造体レイアウト（C++ `ParticleStruct.h` ⇔ HLSL `Particle.hlsli`）

StructuredBufferは**4バイト境界のタイトパッキング**（float3=12B, float4=16B, 16B丸めなし。vector が16B境界を跨いでも可＝①40B版で実機実証済）。C++側 `static_assert` で固定:
```
CSParticleDrawCore  == 36 (translate12 + scaleXY(uint)4 + scaleZ(uint)4 + velocity12 + color(uint)4)
CSParticleSimCore   == 12 (currentTime4 + initialScaleXY(uint)4 + initialScaleZ_isTrail(uint)4)
CSParticleTrail     == 20 (parentIndex4 + lastTrailPosition12 + trailSpawnDistance4)
CSParticleRotation  == 24 (rotation12 + angularVelocity12)
CSParticleOverride  == 8  (uint2)
```
※ scale/initialScale は half3 pack（④）。pack値は個別 `uint` スカラで持ち uint2 のアライメント曖昧さを回避。
※ `gRenderCompact` は `CSParticleDrawCore`(36B) と同型を流用。
※ 旧 `CSParticle`(156B) は `ParticleCSGroupData::particles`(未使用 std::list)の要素型として残置（GPUでは未使用）。

### レジスタ / ルートシグネチャ対応表

**Update CS**（root sig 17 params, `ComputePipeLineManager::CreateUpdateEmitterRootSignature`）
```
param[0..11] UAV : u0=Life u1=DrawCore u2=SimCore u3=Trail u4=Rotation u5=Override
                   u6=FreeListIndex u7=FreeList u8=FreeListTailIndex
                   u9=AliveList u10=AliveCounter u11=RenderCompact
param[12..14] CBV: b0=PerFrame b1=Settings b2=FieldCB
param[15..16] SRV: t0=Fields t1=FieldsOverride
```
C++バインド: `ParticleCSGroup::UpdateParticleCSDisPatch`

**Emit CS**（root sig 17 params, `CreateEmitterRootSignature`）
```
param[0..8]  UAV : u0=Life u1=DrawCore u2=SimCore u3=Trail u4=Rotation u5=Override
                   u6=FreeListIndex u7=FreeList u8=FreeListTailIndex
param[9..12] CBV : b0=EmitterMesh b1=PerFrame b2=Settings b3=FieldCB
param[13..16] SRV: t0=Triangles t1=TriangleCDF t2=Edges t3=Fields
```
C++バインド: `ParticleCSEmitter::EmitterDisPatch`

**Init CS**（root sig 5 params・**構造未変更**, `CreateInitParticleRootSignature`）
```
u0=Life u1=FreeListIndex u2=FreeList u3=FreeListTailIndex / b0=Settings
```
Life(u0)のみ初期化（未発生slotは他バッファを読まれない）。C++: `ParticleCSGroup::InitParticle`

**Count CS**（root sig 3 params・**構造未変更**, `CreateCountRootSignature`）
```
b0=Settings / u0=AliveCount / u1=Life
```
※ Phase2で実ディスパッチは廃止だが起動時コンパイルは走るので gLife 読みに更新。

**描画(Graphics) VS/PS**（root sig 7 params, `PipeLineManager::CreateGPUParticleRootSignature`）
```
param[0] b0 PerView(VS)
param[1] t0 RenderCompact(VS)   ← 描画コンパクション(順次読み)
param[2] t1 texture(PS)
param[3] b1 material(PS)
param[4] t2 AliveList(VS)        ← 回転grの scatter 用
param[5] t3 AliveCount(VS)       ← instanceId カリング
param[6] t4 Rotation(VS)         ← 回転grのみ
```
C++バインド: `ParticleCSEmitter::DrawGraphics` / `DrawGraphicsForPreview`

### 毎フレームのデータフロー（`DrawSystem::Draw` → `ParticleCSEmitter`）
```
[Compute Queue] BeginComputeFrame
  for each group:
    Emit CS (u0-u5 SoA書込, freelistからslot確保)
    → グローバルUAVバリア(pResource=nullptr) ← SoA6本一括同期
    → ResetAliveCounter
    → Update CS (Life早期return → 機能ゲートload → 物理 → SoA store
                 → 生存コンパクション(gAliveList) + 描画コンパクション(gRenderCompact))
    → RecordAliveCountReadback
[Execute] ExecuteComputeCommands
[GPU sync] WaitForComputeOnDirectQueue (commandQueue->Wait, CPUブロックしない)
[Graphics Queue] DrawGraphics
  for each group: FetchAliveDrawCount → drawCount=alive+margin
    → VS: gRenderCompact[instanceId] 順次読み, instanceId>=aliveCount で縮退カリング
```

### CPU-GPU同期モデル（調査済の確定事実）
- `DirectXCommon::PostDraw`: `frameIndex_ = (frameIndex_+1)%kFrameCount` でスロットを毎フレームフリップ。次スロットの前回分が未完了のときだけ `WaitForSingleObject`。= **CPUはGPUより先行でき、毎フレームのCPU-GPU待ちは無い**（Nフレーム分のコマンドアロケータ・バッファリング）。
- `WaitForComputeOnDirectQueue` は GPU側フェンス待ち（CPU非ブロック）。
- `Present(1,0)` で VSync=16.67ms に張り付け。
- **結論**: 300/450万の GPU時間は**純GPU実行時間**。CPU-GPU待ちは既に無い → 後述ダブルバッファのCPU待ち削減効果は無い。

---

## 4. 改修ファイル一覧（このプロジェクトでの①②③④＋Dの全変更）

### D（軽量Updateバリアント）の追加・変更ファイル
| ファイル | 変更概要 |
|---|---|
| `…/CSParticle/UpdateParticleLite.CS.hlsl` | **新規**。演出なし専用の軽量Update（フル版の安価処理のみ／重い演出を除去） |
| `Engine/Utility/Graphics/PipeLine/ComputePipeLineManager.h` | `ComputePipelineType::kUpdateEmitterLite` 追加 + lite PSO生成メソッド宣言 |
| `Engine/Utility/Graphics/PipeLine/ComputePipeLineManager.cpp` | lite PSO生成（root sigはkUpdateEmitterと共有して両キー登録） |
| `…/CSParticle/ParticleCSFieldManager.h` | `GetActiveFieldCount()`（enabledフィールド数）追加 |
| `…/CSParticle/ParticleCSGroup.h/.cpp` | `CanUseLiteUpdate()` 追加、`UpdateParticleCSDisPatch` に `fieldsActive` 引数追加しPSO振り分け |
| `…/CSParticle/ParticleCSEmitter.cpp` | `fieldsActive` を算出して `UpdateParticleCSDisPatch` へ渡す |

### ①②③④の改修ファイル
| ファイル | 変更概要 |
|---|---|
| `Engine/3d/Particle/ParticleStruct.h` | SoA 6構造体 + static_assert、color→uint(40B) |
| `Resources/shaders/Particle/Particle.hlsli` | SoA構造体(PDrawCore等) + Pack/UnpackColorRGBA8。旧`Particle`はscratchで残置 |
| `…/CSParticle/InitParticle.CS.hlsl` | gLifeのみ初期化 |
| `…/CSParticle/EmitParticle.CS.hlsl` | SoA書込 + color pack |
| `…/CSParticle/UpdateParticle.CS.hlsl` | SoA load/store + 機能ゲート + color pack/unpack + **描画コンパクション書込(u11)** |
| `…/CSParticle/CountParticle.CS.hlsl` | gLife読み |
| `…/CSParticle/ParticleCS.VS.hlsl` | **gRenderCompact順次読み** + 回転scatter + color unpack |
| `Engine/Utility/Graphics/PipeLine/ComputePipeLineManager.cpp` | Emit(17)/Update(17) root sig（ループ生成方式） |
| `Engine/Utility/Graphics/PipeLine/PipeLineManager.cpp` | 描画 root sig +t4 Rotation（7 params） |
| `…/CSParticle/ParticleCSGroup.h` | `SoABuffer`構造体 + 7バッファ(soaRenderCompact_含む) + getter |
| `…/CSParticle/ParticleCSGroup.cpp` | `CreateParticleSoABuffers`、Init/Update/Count バインド |
| `…/CSParticle/ParticleCSEmitter.cpp` | Emitバインド、グローバルUAVバリア、描画バインド(RenderCompact+Rotation) |

---

## 5. 実機検証チェックリスト（③描画コンパクション後・最重要）

300万〜450万で起動し:
1. **Draw ms**: 5 → 2ms前後に下がるか（散乱gather解消）。
2. **描画の正しさ**（構造変更のため要確認）:
   - パーティクルが全部出るか／**欠け・チラつき・点滅**が無いか
   - 回転グループ(randomRotation/angularVelocity)が正しく回るか
   - 生存数の急増/急減時（大量Emit直後・gather収束時）に破綻が無いか
3. **色**（②の確認）: randomColor / グラデーション / 中間色 / alphaフェード / trailColor / フィールドcolorMultiply
4. **機能横断**（①の確認）: trail / endScale / override(field) / gather / vortex / curlNoise / 速度ストレッチ
5. **scale（④の確認）**: scaleMin/Max / sinScale / endScale補間 / lifetimeScale で**サイズが正しいか・カクつきが無いか**
   （fp16精度はscale 0.1〜5で~0.05%誤差なので通常は無感。極端に大きい/小さいscaleでのみ要注意）。
   **trail が出るか**（isTrailParticle を SimCore z word の上位16bitに同梱したため、トレイル判定の健全性を要確認）。

起動時assert = ほぼ構造体バイトずれ。欠け/チラつき = 描画コンパクションの整合疑い。サイズ異常/trail消失 = ④の pack 疑い。

---

## 6. 次の候補（★現状診断を受けて優先度改訂・2026-06-27）

**前提**: 帯域律速は卒業し、いまは**レイテンシ/オキュパンシ律速**（§1 ★現状診断）。だから pack(A)はもう効かない。
**最初にやる**: 全パス計測(Emit/Update/Draw/GPU合計)を 4.125M 同条件で取り、③のDraw効果(5→2ms?)を確定する。

### ◎ D. 演出なし専用の軽量Updateバリアント（新・本命）★実装済・実機効果確認（2026-06-28）

> **実機結果（演出なし構成・4.125M）**: Update **8〜10ms → 6.8〜7.1ms**（約20〜30%減）、見た目不変。
> 帯域(load/store量)は軽量版でもフル版と同一なのに Update が縮んだ＝**バイト数ではなくカーネルの重さ(レジスタ→オキュパンシ)が律速だった確証**。診断「レイテンシ/オキュパンシ律速」の踏み絵をクリア。
> → さらにオキュパンシを上げる手として **軽量版のスレッドグループを 1024→256 に縮小**（下記）。Ampere(GA106)は1SM常駐1536上限で、1024ブロックだと占有率67%頭打ちのため。★効果は次回実機計測で確認。

- 単一巨大カーネルが全演出コード(curl/vortex/field/trail/gather/vortex)を抱え**レジスタ過多→低オキュパンシ**になっている疑い。
- 演出が全OFFのグループ用に、**それらをコードごと削った専用 Update CS** を別PSOでコンパイルし、グループの設定でディスパッチを振り分ける。
  レジスタ激減→オキュパンシ上昇→DRAMレイテンシを隠せる見込み。**効けばオキュパンシ律速の確証にもなる**（=これが軽い軽量版で大きく下がるかが踏み絵）。
- 代償: PSO追加 + グループごとのバリアント選択ロジック（中規模の構造変更）。粒度は「全OFF版 / フル版」の2枚。

#### 実装内容（2026-06-28）
- **新シェーダ** `UpdateParticleLite.CS.hlsl`（自己完結・標準版とは別ファイル）。実証済みの `UpdateParticle.CS.hlsl` は**完全無改変**（フル経路のバイト一致を保証）。
- 軽量版が**保持**する処理（いずれも追加バッファloadを伴わない安価な算術）: 加速度 / 重力 / 速度減衰 / 寿命速度減衰 / 移動 / 色補間(3-stop+randomColorアルファ) / スケール(lifetime/end/sin) / 死亡判定+フリーリスト返却 / SoA store / 生存・描画コンパクション。
- 軽量版が**削った**処理: curlNoise / vortex / gather / turbulence / フィールド全般(ApplyFields/ApplySettingsOverride) / トレイル生成(SpawnTrailParticles) / 回転 / `CurlNoise.hlsli`・`Random.hlsli` include ごと除去。
- 触る SoA は **Life(u0)/DrawCore(u1)/SimCore(u2) のみ**（Trail/Rotation/Override は load も store もしない）。死亡返却は u7/u8 のみ（head u6 はトレイル専用なので不使用）。
- **ルートシグネチャはフル版 `kUpdateEmitter` と同一オブジェクトを共有**（軽量シェーダは登録レジスタの部分集合なので合法）。`ComputePipeLineManager` で両キーへ同じ root sig を登録し、PSO だけ差し替え。バインドコード(17 params)は両者で完全同一。
- **適格判定** `ParticleCSGroup::CanUseLiteUpdate(fieldsActive)`: `enableTrail/enableGather/enableVortex/enableCurlNoise/enableTurbulence/enableRandomRotation/enableRandomAngularVelocity` が全 OFF **かつ** `fieldsActive==false` のとき軽量PSO。`fieldsActive = receiveFields_ && FieldManager::GetActiveFieldCount()>0`（有効=enabledフィールド数）。
- 色/スケール/移動の数式はフル版と**完全一致**（適格グループはフル版でも全演出ブランチOFFで同一結果 → lite⇔full 切替で見た目が飛ばない）。設定をImGuiで切り替えると次フレームから自動でPSOが切り替わる。
- **スレッドグループ 256**（フル版1024と別）: Ampere(GA106/RTX3060)は1SM常駐1536スレッド上限のため1024ブロックは占有率67%頭打ち。軽量版はレジスタが少ないので256に割れば最大6ブロック=1536常駐でき占有率を上げられる。Wave集約はワープ単位(32)なのでブロックサイズ非依存・正しさ不変。`kLiteUpdateThreadsPerGroup`(ParticleCSGroup.h) と `[numthreads]` を一致させ、dispatch本数も軽量版だけ256で割る。
- 補足: PIX/Nsight で軽量版の occupancy / achieved bandwidth がフル版より上がっているか確認できれば仮説を直接検証できる（CLIでは取れない）。

#### 実機検証ポイント（D）
1. **演出なしグループ**（重力だけ等）で Update ms がフル版比で下がるか（4.125M で 8〜10ms → ?）。**これが下がればオキュパンシ律速の確証**。
2. 見た目が従来と完全一致するか（色フェード / endScale / sinScale / lifetimeScale / 重力・加速度・減衰 / 死亡タイミング / 描画の欠け・チラつき無し）。
3. ImGui で trail/gather/vortex/curl/turbulence/rotation を ON/OFF した瞬間に破綻なくフル⇔軽量が切り替わるか。
4. フィールドを有効化したグループが**軽量に落ちない**こと（force-trail/override/colorMul が効くか）。
5. 起動時 assert（HLSL は実機DXCコンパイル）= 軽量シェーダのレジスタ宣言ミス疑い。

### A. さらにpack（Update帯域）★④で実施済・**これ以上は非推奨**
- `scale`(DrawCore 40→36B) / `initialScale`(SimCore 20→12B) を `f32tof16`/`f16tof32` 手動packで half3 化（§2④）。
- currentTime/velocity の fp16 化は精度・積分ドリフトのリスク。かつ**もう帯域律速でないため効果小** → 打ち止め。

### B. ダブルバッファ（=Compute(N)とDraw(N-1)の非同期オーバーラップ）
- Draw(~2-5ms)をComputeの裏に隠す。GPU合計 ≈ max(Compute, Draw)。
- **代償**: パーティクルバッファ2セット＝**VRAM2倍（積める上限が下がる＝"もっと出したい"と逆行）**＋描画1F遅延。
- ※単なるバッファping-pong(同フレームA読みB書き)は帯域もCPU待ちも変わらず無意味。効くのは「1F前描画の非同期オーバーラップ」のみ。CPU-GPU待ちは既存で解消済み（§3）なのでそれ由来の利得は無い。

### C. 描画さらに（四分割 / CS描画 等）
- 描画コンパクションで `gRenderCompact` を順次列にした土台を活用。CSでquad展開やタイル分割など。未着手・要設計。

### ◎ E. メモリ削減＝Trail/Rotation/Override の条件付き確保 ★実装済・ビルドOK・実機検証待ち（2026-06-28）
**動機**: 「もっと大量に」はVRAMが上限。1グループの per-particle VRAM は全SoA合計で **148B**
（Life4 + DrawCore36 + SimCore12 + **Trail20 + Rotation24 + Override8** + RenderCompact36 + FreeList4 + AliveList4）。
このうち **Trail/Rotation/Override=52B(35%)** は演出なしグループ（=軽量パス対象＝大量描画したいケース）では完全に無駄。

**実装**: Trail/Rotation/Override を**生成時は1要素ダミー**で確保し、フル版Updateが実際に触るグループだけ
`EnsureUpdateOptionalBuffers(fieldsActive)` が **maxParticleCount で本確保へ作り直す**（毎フレーム冒頭・バインド前）。
- 判定はシェーダのload/storeゲートと一致: needTrail=`enableTrail||fieldsActive` / needRotation=`enableRandomRotation||enableRandomAngularVelocity` / needOverride=`fieldsActive`。
- **ハザードフリー設計**: 再確保時は in-flight が参照中の旧リソース/旧ディスクリプタを**上書きしない**。旧リソースは `retiredSoABuffers_` へ退避（グループ破棄まで生存・ダミーは極小）、ディスクリプタは**新しい枠**を確保して作り直す（SrvManagerはbump割当なので枠は使い捨て）。GPU flush 不要。
- Emit はダミーへ書いても D3D12 robust buffer access で破棄（クラッシュなし）。本確保はUpdate直前なので、本確保した最初の1フレームだけ「その瞬間Emitした粒子のtrail/rotation初期値が0」になる程度の無害な過渡。
- 効果: **演出なしグループ 148B→96B(-35%)**。演出ありグループは必要時に本確保され従来どおり148B（無回帰）。
- 改修: ParticleCSGroup.h/.cpp（SoABuffer に stride/withSrvForVS/allocatedCount 追加, `AllocateSoABuffer`/`EnsureUpdateOptionalBuffers`, `retiredSoABuffers_`, CreateParticleSoABuffers でTrail/Rotation/Overrideを1要素初期化, UpdateParticleCSDisPatch冒頭で Ensure 呼び）。

**VRAM上限の現実（RTX 3060 Laptop=6GB）**:
- 96B/体 × 1億 = **9.6GB** > 6GB → **1億は今のGPUでは不可**（148B時は14.8GB）。
- 実用枠を約4GBとすると **演出なしで約40M(4000万)** が目安（従来148Bは約27M）。1億には実質16GB級GPUが要る。
- **検証点**: 演出なし大量(数千万)でCreateCommittedResourceが通るようになるか / trail・rotation・fieldを後からONにしても破綻なく本確保され機能するか（robust accessで最悪でも無害なはず）/ 見た目据置。
- **さらに削るなら**: RenderCompact(36B)を捨てて描画を散乱gatherへ戻すと60B/体(~66M)だが Draw が悪化（③の逆行）→ 速度↔メモリのトレードオフ。要なら「省メモリ描画モード」をトグルで。

### ◎ F. 生存コンパクションのアトミック競合削減＝グループ単位集約 ★lite実装済・ビルドOK・実機検証待ち（2026-06-28）
**動機**: 35M（演出なし=lite）で Update **135ms** に悪化。35M×144B÷135ms≈**37GB/s**＝理論の1割で**レイテンシ/競合律速**。
本命の疑い＝生存コンパクションが**単一カウンタ `gAliveCounter[0]` に全生存粒子をアトミック追記**（35Mで約110万回/フレームが1アドレスに集中→直列化）。

**実装（lite版のみ。フル版は無改変＝低リスク。演出ありは小規模なので競合は軽い）**:
- 生存コンパクションを**ワープ単位→グループ単位集約**に変更。①各ワープが自ワープ生存数を `groupshared sGroupAliveCount` へ1回 atomic（LDS, 安価）②グループ先頭が**1回だけ** `gAliveCounter[0]` へ atomic して基点取得 ③各生存レーンは `base + ワープ内基点 + レーン内オフセット` へ書き出し。→ グローバル atomic を **1/(グループ内ワープ数=8)** に削減。
- `GroupMemoryBarrierWithGroupSync()` は全スレッド到達必須 → **早期returnを撤廃**し `valid/alive` フラグ方式に再構成（dead/OOBは処理を飛ばして合流。死亡パスのWave集約とload早期スキップは温存）。`SV_GroupIndex` 追加。
- 死亡時 freeList tail の atomic は**低頻度**（死亡数≪総生存数）なので従来のWave集約のまま。
- 改修: `UpdateParticleLite.CS.hlsl` のみ（main再構成＋groupshared 2語）。色/scale/移動/死亡ロジックは無改変。
- **検証点**: 35M で Update が大きく下がるか（下がれば単一カウンタ競合が律速だった確証）/ 見た目・描画の欠け無し / 死亡・生成の健全性。
- **★実機結果(2026-06-28)**: 密35Mで依然 **100ms超**（135msから大きくは下がらず）＝**atomic競合は主因ではなかった**。疎10万でも18〜20ms。→ 真の律速は§1.5の通り「dispatch量（MAX走査）」。本対策は空振り気味。**早期returnを撤廃しバリア化したぶん疎で逆効果の懸念あり**（次セッションで間接ディスパッチ§8を入れればバリアは生存数ぶんに収まり問題化しない。もし§8前に疎を軽くしたいだけなら lite を早期return＋per-warp atomic へ戻す手もある）。
- 真の解 → **§8 生存リスト間接ディスパッチ**（Update を O(生存数) 化）。

---

## 6.5. 残り全候補ロードマップ（「全部やれることはやりたい」2026-06-28）
> ユーザー要望: 最終的にやれる最適化は全部やる。以下を上から順に検討（◎=実装/着手済 ○=有力 △=トレードオフ/対抗馬）。

**Update CS（律速本丸）**
- ★**最優先＝生存リスト間接ディスパッチ（§8に完全仕様）**: Update を O(生存数) 化。§1.5 の dispatch量律速を根本解決。MAX固定コストを消す唯一の手。
- ◎ グループ単位アトミック集約（lite版・§6 F）★実装済だが**空振り**（atomic競合は主因でなかった。§8導入後はバリアが生存数ぶんに収まるので無害）
- ○ 軽量版ローカル `Particle` を必要フィールドだけの**スリム構造体**化 → VGPR削減（§8後の二次対策）
- ○ 同集約を**フル版**にも適用（§8後。演出ありグループ。main を関数抽出して早期return温存しつつバリア対応）
- △ 死亡 freeList tail もグループ集約 / アトミックカウンタのストライピング（必要時のみ）

**Draw**（§8後は Update が軽くなり Draw が相対的に律速になりやすい。ユーザー選択でこの系統に着手）
- ◎ overdraw対策・**距離カリング＋サイズカリング ★実装済・実機OK（2026-06-28）だが当テストシーンでは Draw ms ほぼ不変**＝そのシーンは fillrate/overdraw 律速でなかった。半透明が大量に重なるシーン向けの道具として温存（必要時に ON）。
  - **プレビュー反映の修正済**: プレビューは独立 per-view CB を使うため、当初プレビューではカリングが効かなかった。`DrawGraphicsForPreview` にプレビューカメラ位置(`eye`)・射影(`projScaleY`)・各グループのカリング設定を流し込んで反映するよう修正（`ComputePreviewMatrices` が eye/projScaleY も返す）。**カリング基準=距離カリング:カメラ→粒子のワールド距離 / サイズカリング:画面上の見かけ高さ(NDC)**。
- ◎ overdraw対策・**距離カリング＋距離フェード（詳細）★実装済（2026-06-28）**
  遠い粒子をアルファ 1→0 にフェードし、カリング距離超で**縮退頂点で破棄**（VS早期return＝ビルボード/回転計算もスキップ）。半透明の重なり(ROP/blend)を削減。**設定は PerView に格納**（VSが既に持つCB。巨大な ParticleCSSettings を触らず低リスク）＝`cameraPosition`(Updateで`vp.translation_`コピー)+`enableDistanceCull`/`distanceCullStart`/`distanceCullEnd`+pad×2（PerView 144→176B, C++/HLSL手動一致・CB16B straddle無し）。**デフォルトOFFで既存挙動完全不変**。改修=ParticleStruct.h/Particle.hlsli(PerView+6field) / ParticleCS.VS.hlsl(unpack直後に距離cull/fade) / ParticleCSGroup.cpp(Update で cameraPosition、DrawImGui に「描画カリング」節) / ParticleCSEmitter.cpp(save/load 両経路)。**検証点=起動assert無し(PerViewレイアウト一致)/デフォルトで従来一致/ON で遠粒子フェード&カリング/start・end 調整/保存ロード永続/大量・遠距離半透明で Draw ms 低下(GpuProfiler)**。
- ◎ overdraw対策・**画面サイズ上限＋微小カリング ★実装済・ビルドOK・実機検証待ち（2026-06-28）**
  距離カリングと VS・PerView の土台を共有。`projScaleY`(=projection[1][1], Updateでコピー)で粒子中心の画面NDC高さ `worldHalf*projScaleY/centerW` を概算。①**微小カリング**: 画面高さ<`minScreenHeight`(0=無効)はサブピクセル相当として縮退カリング。②**画面サイズ上限**: 画面高さ>`maxScreenHeight`(NDC, 2=全画面)の巨大粒子は world スケールを一律縮小し1粒子のfillrate暴発を抑える。worldMatrix構築前(pScale使用前)に実施。`enableSizeClamp=0`で既存挙動不変。PerView pad×2を実フィールド化(projScaleY/enableSizeClamp/maxScreenHeight/minScreenHeight)し 176→192B(C++/HLSL手動一致)。改修=ParticleStruct.h/Particle.hlsli/ParticleCS.VS.hlsl/ParticleCSGroup.cpp(Updateでproj、ImGui「画面サイズ制限」)/ParticleCSEmitter.cpp(save/load)。検証点=起動assert無/デフォルト一致/ON で巨大粒子が縮む&微小粒子消える/巨大半透明シーンでDraw ms低下。
- ○ ③描画コンパクションの Draw 5→2ms 効果を**全パス計測で確定**（未測定）
- ○ CS描画 / quad展開 / タイル分割（`gRenderCompact` 順次列の土台を活用）

**Emit**
- ○ Emit も**軽量バリアント**化（演出なしグループ）
- ○ Emit が常時書く Trail/Rotation/Override を**ゲート**（条件付き確保と整合、ダミーへのOOB-drop無駄を排除）

**メモリ（積める上限）**
- ◎ Trail/Rotation/Override 条件付き確保（§6 E, 148→96B）★実装済
- ○ 非回転グループで `gAliveList` 省略（描画は RenderCompact 順次読みのみ。回転grだけ aliveList 必要）→ -4B/体
- △ 省メモリ描画モード: RenderCompact 廃止で 60B/体(~66M)だが Draw 悪化

**計測/検証基盤**
- ○ 全パス計測（Emit/Update/Draw/GPU合計）＋生存数を 10/20/35M で取得しボトルネック確定
- ○ PIX/Nsight で occupancy / achieved bandwidth 実測（仮説の直接検証・CLI不可）

**対抗馬（優先度低）**
- △ B. ダブルバッファ（Compute(N)/Draw(N-1)オーバーラップ）= VRAM2倍で「もっと出したい」と逆行

---

## 7. 用語・場所メモ
- オーケストレーション: `ParticleCSEmitter::DrawCompute/DrawGraphics`、束ねは `DrawSystem::Draw`。
- リソース生成/バインド: `ParticleCSGroup`（`CreateParticleSoABuffers`等）。
- ルートシグネチャ: Compute=`ComputePipeLineManager`、描画=`PipeLineManager::CreateGPUParticleRootSignature`。
- SRVヒープ上限 `kMaxSRVCount=49152`（グループあたりSoAで8descriptor程度消費だが余裕大）。

---

## 8. 【完了】生存リスト間接ディスパッチ ★Step1-3 実装＆実機OK（2026-06-28）= Update を O(生存数) 化

> **結論**: Step1(ping-pong基盤)→Step2(Emit append + Update を listIn 入力に)→Step3(dispatch 本数を in リスト長由来に) を実機1ステップずつ確認しながら投入完了。§1.5 の「dispatch量律速(MAX固定コスト)」を根本解決。全実装ビルドOK・未コミット(feature/player)。
> **§8で landscape 変化**: aliveList が「描画の回転scatter用」から「**間接dispatchの listIn/listOut そのもの**」に役割変化 → §6.5「非回転grで aliveList 省略」は**陳腐化(aliveList 必須)**。ping-pong で aliveList/counter が各 ×2(+約4B/体, 96→100B/体・許容)。**次の律速は要計測**。

> ### 実装進捗（実機で1ステップずつ確認しながら投入中）
> - **Step3 = dispatch 本数を in リスト長由来に（＝O(生存数) 化・性能の本命）★実装済・ビルドOK・実機検証待ち（2026-06-28）**
>   `ParticleCSGroup::UpdateParticleCSDisPatch` の dispatch 本数を `maxParticleCount` 固定から **in リスト長の推定値由来**へ。in リスト長 = 前フレーム out カウンタを `FetchAliveDrawCount()` で readback した `aliveDrawCount_`（1〜2F遅延）。
>   `threadCount = inLenEst + inLenEst/4 + emitCount + 4096`（25% + 当フレーム emit + 定数）を **maxParticleCount でクランプ**。GPU 側は `tid >= gAliveCounterIn[0]` で余剰スレッドを捨てるので **over-dispatch は無害**。
>   ★**under-dispatch は厳禁**（in リスト長より少なく dispatch すると未処理粒子が out に積まれず slot が永久に漏れる＝描画の取りこぼしと違い**自己回収しない**）。だから margin は安全側。lite/no-trail は成長=emitのみで安全。**フル版のトレイル大量スパイクは 25% を超える成長で稀に取りこぼす懸念**（定常状態は readback が追従し 25% で足りる。バーストで消失が見えたら margin を `+ inLenEst*maxTrailPerParticle` 等へ増やす）。初回Fは inLenEst をゼロInit/異常値ガードで maxClamp、かつ in カウンタ=0 で Update 即return＝無害。
>   改修=`ParticleCSGroup.cpp`（UpdateParticleCSDisPatch の dispatch 本数計算のみ。C++ のみ）。
>   - **検証点**: ①起動assert無し（C++のみ低リスク）②見た目が Step2 と同じ（**粒子の取りこぼし＝消失が無いか**。特にトレイル・大量Emit直後）③**疎(生存少/MAX大)で Update ms が激減**するか GpuProfiler で計測（35M MAX×生存10万で 18〜20ms → ~0.1ms 近辺を狙う＝§1.5 の MAX固定コスト消滅の確証）④密(大量生存)でも見た目正常（dispatch が maxCount にクランプ＝従来同等）。
> - **Step2 = Emit append + Update を listIn 入力に切替（本丸）★実装済・ビルドOK・実機検証済（2026-06-28）＝問題なし**
>   メモ検証順(2)+(3)を**まとめて**実装（Emit appendだけだとUpdateの全走査と二重カウントするため不可分）。dispatch本数は**まだ maxCount のまま**（perf payoff は Step3）。bounds で守る。
>   - **Emit**(`EmitParticle.CS.hlsl` + root sig 17→20): 末尾で `InterlockedAdd(gAliveCounter[0])` → `gAliveList[dst]=particleIndex` / `gRenderCompact[dst]=dc` を append（u9/u10/u11 追加）。
>   - **Update full**(`UpdateParticle.CS.hlsl` + root sig 17→19): 入力を `tid=DTid.x; if(tid>=gAliveCounterIn[0])return; particleIndex=gAliveListIn[tid]` に変更（t2=listIn/t3=counterIn 追加）。**早期return可**(Wave集約のためバリア制約なし)。sim本体は無改変。**トレイル子も out へ append**（SpawnTrailParticles 内で `gAliveList`/`gRenderCompact` へ追記。append しないと in リスト経由でしか sim しない設計で子が処理も描画もされない＝要注意点）。survivor 末尾コンパクションは無改変（共有 out カウンタへ続けて積む）。
>   - **Update lite**(`UpdateParticleLite.CS.hlsl`): 同様に listIn 入力化。**早期return不可**(グループ集約バリア)なので `particleIndex=-1` 既定＋`if(tid<counterIn)`内でセット、dead/OOBは alive=false で合流。lite はトレイル無しなので append は survivor のみ。
>   - **オーケストレーション**(`ParticleCSEmitter::DrawCompute`): per-group ループから**パス構成**へ再編。①各グループ AdvanceAliveFrame+Reset(out) → ②Emit一括(append out) → ③グローバルUAVバリア → ④各グループ Update(read in/append out) → ⑤各グループ Readback(out)。**reset を Emit より前**に移動（Emit が out カウンタへ append するため必須）。これで潜在的な Emit の N²（per-group ループ内で全グループ Emit）も解消。
>   - **C++バインド**: Emit が u9/u10/u11=out リスト/カウンタ/renderCompact を常時バインド(`ParticleCSEmitter`)。Update が t2/t3=**in フェーズ**(`alivePhase_^1`)の SRV をバインド(`ParticleCSGroup`)。in/out は別物理バッファなので読み書きハザード無し（UAV状態のバッファをSRV読みするのは既存の描画VS t2/t3と同じ実証済みパターン）。
>   - フロー: out=[今FのEmit]+[今Fのトレイル子]+[Update後survivor]。次FのUpdateがin=前Fのoutを処理。drawCount=out カウンタ readback。初回FはinカウンタがCreateCommittedResourceのゼロ初期化で0→Updateは即return、Emit分だけ描画→自己整合。
>   - **検証点(最重要)**: 起動assert無し / 全パーティクルが従来どおり描画(欠け・チラつき・点滅無し) / **トレイルが正しく出るか**(子append) / randomColor・グラデ・中間色・alphaフェード / フィールド全般(gather/vortex/curl/turbulence/override/force-trail/colorMul=full経路) / 回転グループ / 大量Emit直後&収束時に二重カウント/取りこぼし無し / lite(演出なし)とfull(演出あり)両経路 / ImGuiでlite⇔full切替も破綻無し。※**この時点ではperfは不変**(dispatchまだmaxCount)＝correctness確認のステップ。通れば Step3(dispatch本数=counterIn_cpu)で疎のUpdate激減を取る。
> - **Step1 = 生存リスト ping-pong 基盤（C++のみ・挙動不変）★実装済・ビルドOK・実機検証済(2026-06-28)＝問題なし**
>   `aliveList`/`aliveCounter` を `listBuf[2]`/`counterBuf[2]` の ping-pong 化し、`alivePhase_`(out=phase / in=1-phase) を毎フレーム反転(`AdvanceAliveFrame`)。
>   Reset/Update(u9,u10)/Readback/Draw(t2,t3) はすべて **out フェーズ**を参照。**シェーダ/ルートシグネチャは無改変**、Update は従来どおり全スロット走査で out を毎フレーム作り直すため**見た目は完全に不変**（どちらの物理バッファに書いても描画は out を読むだけ）。readback は out からコピーする共有1個。
>   改修=`ParticleCSGroup.h`(ping-pong配列[2]+`alivePhase_`+`AdvanceAliveFrame`+getterをout参照に) / `ParticleCSGroup.cpp`(`CreateAliveListResources`を2枚ループ確保, Reset/Update/Readbackバインドを`[alivePhase_]`に) / `ParticleCSEmitter.cpp`(DrawCompute冒頭=Resetの直前で`AdvanceAliveFrame`)。
>   VRAM: aliveList/aliveCounter がもう1枚ずつ（+約4B/体）。**検証点=起動assert無し / 見た目・欠け・チラつき無しで従来と完全一致**。これが通れば Step2(Emit append+Update listIn入力) へ。
> - Step2以降は下記「検証順」「触るもの」に従って未着手。

> **目的**: Update を「全 maxParticleCount スロット走査」から「生存粒子リストだけ処理」に変え、Update コストを **O(生存数)** にする。MAX を大きくしても疎なら ~0.1ms。§1.5 の dispatch量律速を根本解決する唯一の手。
> **なぜ別セッションか**: Emit/Update(lite,full)/描画/オーケストレーション/ルートシグネチャ/カウンタにまたがる横断改修で、HLSLは実機起動時コンパイル。ノーテスト一括投入は起動assertリスク大。**実機で1ステップずつ起動確認しながら**入れること。

### コア設計（ExecuteIndirect は使わず CPU カウントでディスパッチ＝シンプル）
**生存スロットの「処理リスト」を毎フレーム作り、ping-pong で読み書きする。**
- `listBuf[2]`（uint×maxCount, ping-pong）: 処理対象スロットindexの列。
- `counterBuf[2]`（uint, ping-pong）: そのリスト長。
- フレーズ `phase`(0/1) を毎フレームトグル。`in=phase`, `out=1-phase`。
- リスト統一方針: **out リスト = [今フレームEmitした粒子] + [今フレームUpdate後も生存した粒子]**。これが次フレームの in になる。
- `gRenderCompact`（既存, draw用）は out と**同じ idx** で書く（Emitも書く）→ 描画は今フレーム emit 分も含めて即描画（1F遅延なし）。drawCount = counterBuf[out]。

### 毎フレームのフロー（DrawCompute, グループごと）
```
in = listBuf[phase],   counterIn  = counterBuf[phase]
out= listBuf[1-phase], counterOut = counterBuf[1-phase]
1) counterOut を 0 にリセット（ResetArgs CS 流用, Emit より前）
2) Emit: 空きスロット確保→SoA[slot]書込→ idx=InterlockedAdd(counterOut,1);
         listOut[idx]=slot; gRenderCompact[idx]=（emitした粒子のDrawCore）
3) UAVバリア
4) Update: dispatch本数 = ceil((counterIn_cpu + margin)/256)   ※counterIn_cpuは前フレームreadback値(1〜2F遅延)+余裕
     thread tid: if tid >= counterIn[0] → alive=false で合流(早期return不可・バリア用)
                 else slot=listIn[tid]; SoA[slot] load→sim→store;
                      生存なら idx=InterlockedAdd(counterOut,1)（グループ集約のまま）;
                      listOut[idx]=slot; gRenderCompact[idx]=odc
                 死亡なら freeList へ返却（従来通り）
5) counterOut を readback（次フレームの dispatch本数＋drawCount に使う）
6) phase ^= 1
```
- **margin**: counterIn_cpu は遅延読戻しなので、急増時に取りこぼさないよう `+25% +4096` 程度（既存 drawCount と同方針）。取りこぼしても次フレームで回収＝自己修復。
- **描画**: drawCount = counterBuf[out]_cpu + margin。VS は `instanceId >= counterBuf[out][0]`(fresh GPU) でカリング（既存と同型）。回転scatterは `gRotation[listBuf[out][instanceId]]`。

### 触るもの（チェックリスト）
1. `ParticleCSGroup.h/.cpp`: listBuf[2]/counterBuf[2] 生成（ping-pong, UAV＋VS用SRV）, getter, phase, reset対象をout-counterに, dispatch本数を `counterIn_cpu` 由来に, readbackをout-counterに。`gAliveList`/`gAliveCounter` を ping-pong 化（実質これらを2枚に）。
2. `EmitParticle.CS.hlsl` + Emit root sig: listOut(UAV)+counterOut(UAV)+gRenderCompact(UAV) を追加し、emit末尾で append（idx確保→listOut/renderCompact書込）。root params 17→20。
3. `UpdateParticle.CS.hlsl`(full) / `UpdateParticleLite.CS.hlsl`(lite) + Update root sig: 入力を `listIn[tid]`(SRV or UAV)＋`counterIn`(SRV)に、出力 append 先を listOut/counterOut に。`slot=listIn[tid]` 以外の sim 本体は無改変。lite はグループ集約バリアをそのまま使える（dispatchが生存数ぶんなのでバリアも軽い）。
4. `ParticleCSEmitter::DrawCompute`: 順序を **reset(out)→Emit→barrier→Update→readback(out)→phase反転** に。Emit が append するので reset は Emit より前。
5. 描画 (`DrawGraphics`): drawCount/カリングを counterBuf[out] に、回転scatterを listBuf[out] に。
6. `ComputePipeLineManager`: Emit/Update root sig のパラメータ数更新。

### 注意・罠
- **早期return不可**（lite のグループ集約バリアのため）。`tid>=counterIn[0]` は alive=false で合流させる（§6 F で対応済の構造を踏襲）。
- **ping-pong必須**: listIn と listOut を同一バッファにすると read/write 競合（compactedIdx≤tid で書込が他threadのread前に来る）。必ず2枚。
- **counterIn は GPU 値で bounds、CPU 値で dispatch本数**。CPU値は遅延なので margin 必須。
- 新規 emit 粒子は out に入る→**次フレームの Update から sim 対象**（生成フレームは Emit が renderCompact に書くので描画はされる）。trail 子も同様（既存の1F遅延と整合）。
- VRAM: ping-pong で listBuf がもう1枚（+4B/体）。96B→100B/体。許容。
- これが入れば §1.5 の「MAX固定コスト」が消え、疎18〜20ms→~0.1ms、密は生存数相応（35M密は依然5GB/frameで重いが、それは物理限界＝MAXを実需に合わせる運用で回避）。

### 検証順（実機・1ステップずつ起動確認）
(1) buffers/ping-pong追加だけ入れて従来動作維持で起動assert無し確認 → (2) Emit append → (3) Update を listIn 入力に切替（dispatchは当面 maxCount のままでも可、bounds で守る）→ (4) dispatch本数を counterIn_cpu 由来に → (5) 疎でUpdate激減を確認 → (6) 描画カリング/回転を out 側に。各段で trail/randomColor/gather 等の見た目健全性を確認。
