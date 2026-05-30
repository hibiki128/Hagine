# GPUParticle 改修 まとめ

## 全体状況

- 計画ファイル: `~/.claude/plans/nifty-drifting-river.md`（9フェーズ構成）
- 現状 **未コミット**（任意のタイミングでコミット可）。問題が出たら `git checkout .` でベースラインに戻せる（新規ファイルは `git clean` 対象）。
- ビルド: `MSBuild.exe Hagine.sln -p:Configuration=Debug -p:Platform=x64`
  - **Git Bash 経由ではスラッシュ引数 `/p:` がパスに誤変換される**ため `-p:` 形式を使う。
  - MSBuild パス: `C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe`
  - x64 のみ。Debug / Release 両方クリーンビルド確認済み。
  - HLSL は実行時に DXC でコンパイルされるため、シェーダのエラーはビルドに出ず実機起動時に assert する点に注意。

---

## 完了したフェーズ（実機確認 OK 済み）

### Phase 1 — 死スロット描画の撲滅（生存コンパクション）

- 描画が常に全スロット（最大10万）instance 発行していたのを、生存数ぶんだけに削減。
- `UpdateParticle.CS` が生存 slot を `aliveList` へ詰め、VS は `aliveList` 経由で参照＋`aliveCount` で `instanceId >= 生存数` を確実にカリング。
- `ResetArgs.CS`（1スレッド）で生存数カウンタを毎フレーム 0 にリセット。
- 描画 instanceCount は `aliveCounter` の readback 値（1〜2F遅延）＋マージン。VS 側で正確カリングするのでズレても安全。
- **設計判断**: `ExecuteIndirect` ではなく「コンパクション＋VSカリング」で実装。compute→direct のキュー跨ぎでの indirect-argument 状態遷移の危険を回避するため。
- 主な変更ファイル: `Particle.hlsli`, `UpdateParticle.CS.hlsl`, `ParticleCS.VS.hlsl`, 新規 `ResetArgs.CS.hlsl`, `ComputePipeLineManager.*`, `PipeLineManager.cpp`, `ParticleCSGroup.*`, `ParticleCSEmitter.cpp`

### Phase 2 — CountParticle 全廃 ＋ トレイル親の global 往復除去

- 毎フレームの全N集計ディスパッチ（`CountParticle.CS`）を廃止し、`GetAliveParticleCount` を `aliveCounter` 読み戻しに統合。
- `SpawnTrailParticles` を `inout Particle p` 化し、親の `gParticles[i]=p; func(); p=gParticles[i]` 往復を除去（子スロットのみ直書き）。
- **設計判断**: 計画にあった「freeList の LIFO 化」は**取り下げ**。現状のリングバッファ（head/tail 分離）は Update 内で death(push)/trail(pop) が同時実行されても領域が分離され安全で、単純 LIFO はかえってハザードを生むため。`DispatchIndirect`（sim を生存数化）は ping-pong が前提のため Phase 3 へ移動。
- 主な変更ファイル: `UpdateParticle.CS.hlsl`, `ParticleCSGroup.cpp`, `ParticleCSEmitter.cpp`

### Phase 4 — über シェーダ整理

- `ApplySettingsOverride` を `inout Particle p` 化し、設定上書きの global 往復を除去。
- 結果、`UpdateParticle.CS` の親パーティクルは **`gParticles[i]` のロード1回・ストア1回のみ**（grep 確認済み）。「肥大化で重い」の実コスト源だったメモリ往復を解消。
- **設計判断**: 「enable のビットマスク化」は意図的に見送り。定数バッファ由来の uniform 分岐（全スレッド同一経路）でコストがほぼ無く、ビットマスク化は負荷を減らさずリスクだけ増やすため。
- 主な変更ファイル: `UpdateParticle.CS.hlsl`

### Phase 7 — クローンのプール化（＋クラッシュ修正）

- **核心問題**: `RemoveUnusedIndependentGroups` がどこからも呼ばれておらず、シーン内で Clone/AddParticleGroup する度に独立グループ（各 ~15MB の GPU バッファ＋SRV）が累積し、シーン終了まで解放されていなかった（弾100発 = 1.5GB 相当）。
- **対応**: `GetIndependentParticleGroup` をテンプレート名キーの**再利用プール**化（空きがあれば GPU バッファ/SRV を再確保せず `ResetForReuse`=InitParticle で状態だけリセット）。`ReleaseIndependentGroup` 新設＋`~ParticleCSEmitter()` でエミッタ破棄時にプール返却。`ClearIndependentGroups` は破棄せずプール退避（上限 `kMaxPooledPerTemplate=32`）。
- → メモリ使用量が「同時生存数のピーク」で頭打ちに。遷移順序（旧エミッタ破棄 → ClearIndependentGroups → 新シーン生成）によりエイリアシング無し。
- **クラッシュ修正**: 終了時の `std::length_error`（`Framework::Finalize` の順序で、グループ破棄後にエミッタのデストラクタが破棄済みグループに `GetGroupName()` を呼びダングリング参照していた）を、`ReleaseIndependentGroup` をポインタ比較のみ・deref しない設計に変更して解消。
- 主な変更ファイル: `ParticleCSGroupManager.h`, `ParticleCSGroupManager.cpp`, `ParticleCSGroup.h`, `ParticleCSEmitter.h`, `ParticleCSEmitter.cpp`

---

## 進行中

### Phase 8 — Effekseer 風プレビュー窓（暗い空間＋白グリッド）

> ステータス: 8a-min が実機OK。**8a-grid / 8b / 8c ＋ プレビュー窓の統合UI化（画像の可変サイズ・エディタ統合）をコード実装・Debug/Release 両ビルド確認済み（実機確認待ち）**。Phase 8 はコード上は完了。
> 次にやること: 実機（DemoScene）で「表示 > ウィンドウ > パーティクルプレビュー」を開き、①ウィンドウ可変で画像が追従するか ②右パネルでエミッタ/グループ作成・選択・動き設定ができるか ③選択エミッタのパーティクルがプレビューVPで出るか ④オービット/ズーム/再生/一時停止/背景色/グリッド を目視確認。問題なければ実機OKに更新。残るは Phase 3/5/6（未着手）。

**8d 実装済み（ビルド OK・実機確認待ち）— プレビュー窓の統合UI化**
- **画像サイズの可変化**: RT/深度を**クライアント解像度ぶん（1760×990）で最大確保**し、毎フレーム ImGui ビューポート子領域のサイズを実描画サイズ（`previewRenderWidth_/Height_`）として記録。`RenderPreview` はその左上部分のみにビューポート＋シザーで描画し、`ImGui::Image` は UV 部分表示（`uv1 = renderSize/maxSize`）。→ **リソース再確保なし**で frame-latency ハザードを避けつつ、ウィンドウサイズに追従。射影アスペクトも実描画サイズから算出（歪みなし）。
- **エディタ統合**: プレビュー窓を左右分割（左=ビューポート子＋再生ツールバー、右=エディタ子）。右パネルに**カメラ／表示設定 ＋ `ShowImGuiEditor()`（作成タブ）＋ `DebugAll()`（選択コンボ＋選択エミッタの動き設定タブ）**を全部入れた。`BeginTabBar("GPUパーティクル")` 同名IDの2回呼びは同一タブバーへマージされる既存挙動を踏襲。
- **重複排除**: `BaseScene::DrawParticleEditorUI` の旧「GPUパーティクル」ウィンドウ（`ptCSEditor_->ShowImGuiEditor()/DebugAll()`）を撤去し、GPUパーティクル編集UIはプレビュー窓へ一本化。CPU側（CPUパーティクル窓）は不変。
- 主な変更ファイル: `ParticleCSEditor.h/.cpp`, `BaseScene.cpp`

**8e 実装済み（ビルド OK・実機確認待ち）— GPUパーティクルエディタを全シーンで駆動**
- **問題**: `ptCSEditor_` の Compute/Graphics は **DemoScene だけが `drawSystem_->Register`** していたため、他シーンでは編集エミッタがシミュレート/描画されず、プレビューにも出なかった。
- **対応（シーン非依存の全体駆動）**: `DrawSystem::Draw` に直接組み込み。
  - Compute フェーズで `ParticleCSEditor::DrawAllCompute(vp)` を常時呼ぶ。`ExecuteComputeCommands` は記録が無ければ自己ガード（`computeListIsOpen_`）で no-op、`WaitForComputeOnDirectQueue` も signaled 済み値への待ちで無害 → 全シーンで安全に常時実行。
  - stage0 ループで `ParticleCSEditor::DrawAllGraphics(vp)` を常時呼ぶ（シーン offscreen へ in-scene 描画）。`stageOffScreens_[0]` は `Initialize` で設定され `Clear()` でも消えないため stage0 は常に存在。
- **重複排除**: `DemoScene` の `DemoScene_Compute` 登録と `DemoScene_All` 内の `ptCSEditor_->DrawAllGraphics` を撤去（二重 Update/Compute/Draw を防止）。他シーンは元々未登録なので二重化なし。
- → これで**どのシーンでも**編集エミッタが発生・シミュレート・描画され、プレビュー窓でも確認可能に。
- 主な変更ファイル: `DrawSystem.cpp`, `DemoScene.cpp`

**前提整備済み**
- `DirectXCommon`: RTV ヒープ 6→8、DSV 1→2 に拡張（slot6 = プレビュー色RT, slot1 = プレビュー深度用）。既存スロットは不変。

**8a-min 実装済み（実機 OK）**
- 専用色RT（R8G8B8A8_UNORM_SRGB / 512×512 / RTV スロット6）＋ ImGui 表示用 SRV を `ParticleCSEditor::Initialize`（Framework:170）で生成。
- `DrawSystem::Draw` 冒頭（direct リスト記録中・ステージ束ね前なので RT 復元不要）で `RenderPreview()` ＝ 暗色クリア → SRV 遷移。
- `ShowImGuiEditor` 内で `ImGui::Image` 表示「CSパーティクル プレビュー」窓。
- 主な変更ファイル: `DirectXCommon.cpp`, `ParticleCSEditor.h`, `ParticleCSEditor.cpp`, `DrawSystem.cpp`

**8a-grid 実装済み（ビルド OK・実機確認待ち）**
- 専用深度バッファ（D24_UNORM_S8_UINT / 512×512 / DSV スロット1）を `DirectXCommon::CreateAdditionalDepthResource`（新規 public ラッパ、private `CreateDepthStencilTextureResource` を呼ぶ）で生成し `InitializePreview` で DSV 作成。サンプリングしないので常時 DEPTH_WRITE（遷移不要）。
- **共有 `DrawLine3D` との衝突回避の設計判断**: `DrawLine3D` はシングルトンで頂点バッファ／viewProject CB が1つきり。メインシーンは Update でライン群をキューし stage0（`DrawSystem.cpp:159`）で1つの VP で描画＋Reset する。プレビューは別 VP・別タイミングが必要なため、`DrawLine3D` を再利用すると Reset でメインのキューを破壊してしまう。→ **プレビュー専用の独立 VB＋専用 viewProject CB を `ParticleCSEditor` に持ち、PSO だけ既存 `kLine3d` を流用**して描画（`PipeLineManager::DrawCommonSetting(kLine3d)` → `IASetVertexBuffers` → `SetGraphicsRootConstantBufferView(0, cb)` → `DrawInstanced`）。共有バッファには一切触れない。
- グリッドは XZ 平面 20分割 / halfSize=10、中央線を赤(X軸)・青(Z軸)で色分け（`BuildPreviewGrid`、初回1回だけ頂点生成）。
- カメラは球面座標のオービット（`ComputePreviewViewProjection`：`MakeRotateMatrix`+`Inverse` で view、`MakePerspectiveFovMatrix` で proj）。yaw/pitch/distance はメンバ保持（8c でマウス操作に接続予定）。
- `RenderPreview` で色RT＋専用DSV を束ね、暗クリア＋深度クリア、プレビュー解像度の viewport/scissor を設定してグリッド描画。後続ステージは `PreRenderTexture` が全画面 viewport を再設定するので復元不要。
- 主な変更ファイル: `DirectXCommon.h/.cpp`, `ParticleCSEditor.h/.cpp`

**8b 実装済み（ビルド OK・実機確認待ち）— 選択エミッタの隔離描画**
- **設計判断（共有リソースの VP を汚さない）**: エミッタのパーティクルグループは per-view CB（`viewProjection`/`billboardMatrix`）を1つ持ち、メインシーン描画と共有する。`group->Update(vp)` が compute フェーズでメイン VP を書き込むため、プレビューで同 CB を上書きするとメイン描画が壊れる。→ `ParticleCSEditor` が**プレビュー専用 per-view CB**を持ち、エミッタに `DrawGraphicsForPreview(perViewGpuAddress)`（DrawGraphics の複製・root param0 だけ差し替え・ワイヤー DrawEmitter は描かない）を新設して隔離描画。
- **描画タイミング**: `RenderPreview()` を `DrawSystem::Draw` の **Compute フェーズ完了後**（`ExecuteComputeCommands`/`WaitForComputeOnDirectQueue` 後・シャドウ/ステージループ前）へ移動。compute 済みの生存バッファが VS 読み取り可能な状態のまま、プレビューVPで再描画する。パーティクル PSO は SRV テーブルを使うので `SrvManager::SetDescriptorHeap()` を束ね直す。
- ビルボードはプレビュー view から計算（`group->Update` と同ロジック）。enableBillboard=1/velocityStretch=0 固定（per-group 差は単一CBのため未対応、必要なら後日 per-group CB 化）。
- 主な変更ファイル: `ParticleCSEditor.h/.cpp`, `ParticleCSEmitter.h/.cpp`, `DrawSystem.cpp`

**8c 実装済み（ビルド OK・実機確認待ち）— カメラ操作・各種設定・ウィンドウ管理の仕上げ**
- **オービットカメラ**: `ImGui::Image` 上で **左ドラッグ=回転（yaw/pitch、pitch ±89°クランプ）／ホイール=ズーム（距離 1〜100）**。`IsItemHovered()` で画像上のみ反応。カメラ UI（距離/注視点/リセット）。
- **再生 / 一時停止**: 選択中エミッタの `GetAuto`/`SetAuto` で自動発生をトグル（再生/一時停止ボタン）＋`EmitOnce` の単発ボタン。エミッタに `GetAuto()` ゲッターを追加。※compute はシーン全体で走るため、これは「選択エミッタの emit を止める」挙動。
- **背景色 / グリッド設定**: 背景色 `ColorEdit3`（`RenderPreview` の clear 色に反映）、グリッド表示トグル・分割数(2〜100)・半径・色。グリッドVBは**最大容量で永続マップ**し、設定変更時に dirty フラグ→`RenderPreview` 内で内容のみ書き換え（DrawLine3D と同じ毎フレーム書き換えパターン）。
- **ウィンドウ表示管理を ImGuiManager に集約**: プレビュー窓の ON/OFF を **MainMenuBar「表示 > ウィンドウ > パーティクルプレビュー」**(`showParticlePreviewView_`) で管理。`ImGuiManager::ShowParticlePreviewWindow()` がフラグを見て `ParticleCSEditor::ShowPreviewWindow(&flag)` を呼ぶ（ウィンドウのXボタンとメニューが連動）。`ShowMainUI` の描画ループに追加、`Save/LoadFlag` で永続化（`Frags.json`）。`ParticleCSEditor::ShowImGuiEditor` 内の旧 `ShowPreviewWindow()` 直呼びは撤去。
- Debug/Release 両方クリーンビルド確認済み。
- 主な変更ファイル: `ParticleCSEditor.h/.cpp`, `ParticleCSEmitter.h`, `ImGuiManager.h/.cpp`

---

### Phase 6 — 環境フィールド整理（低リスク分を実装済み・ビルドOK）

> ユーザー選択により **GPUバイトレイアウトを維持したまま**の低リスク版を実施（runtime 挙動は不変）。
> override の global 往復除去は Phase 4 で完了済みのため、本フェーズ残りは「責務の明文化」「レイアウト契約の固定」「仕様明文化」が中心。

- **GPUレイアウト契約の固定（最大の安全価値）**: `ParticleFieldData`（C++）に `static_assert`（`sizeof==112` ＋ `colorMultiplier=60 / enableSettingsOverride=76 / groupId=96` の offsetof）を追加。HLSL `struct ParticleField`（`Particle.hlsli`）とのバイト一致を**ビルド時に強制**。これまで「動いているから一致しているはず」だった暗黙契約を機械チェック化（この領域の最大の事故源＝サイレントなレイアウトずれを封じた）。`<cstddef>` を include。
- **責務の明文化（物理分割はせず論理整理）**: `ParticleFieldData` に 7 責務（Force / LifeDrain / ForceTrail / ColorMultiply / SettingsOverride / EmitSpawn / GroupFilter）のヘッダコメント＋各メンバを責務ごとにコメント区切り。struct の物理分割はレイアウト維持の制約と全 CPU 呼出し側への波及リスクを避けて**見送り**（runtime 安全優先のユーザー選択に合わせた）。
- **グループフィルタ仕様の明文化**: GPU `ApplyFields` のフィルタ規則（`field.groupId==-1` / `emitter.fieldGroupId==-1` / 一致時のみ）を C++ struct 側コメントにも記載し単一情報源化。HLSL 側にも C++ との一致義務コメントを追加。
- **kMaxFields 見直し**: 値は 8 のまま（不足の根拠なし）。fields/override 両バッファ容量・AddField 上限・シェーダ fieldCount クランプを兼ねる旨と、増やしても挙動互換である旨を明記。
- Debug/Release 両ビルド確認済み（static_assert が通る＝レイアウト計算が正しい）。
- 主な変更ファイル: `ParticleStruct.h`, `Particle.hlsli`, `ParticleCSFieldManager.h`

## 未着手フェーズ

- **Phase 3** — ダブルバッファ(ping-pong aliveList) ＋ DispatchIndirect で sim を生存数化。
  - 保留理由: Phase 1/2 完了後、高密度時（生存数≈最大数）は sim が実質すでに生存数比例で、追加 perf 効果が小さい一方、コマンドシグネチャ/indirect 引数/ping-pong 管理で最高リスク。ハザード解消＝ダブルバッファ自体の価値はある。**実機検証必須のため未着手。**
- **Phase 5** — トレイル再設計（freeList pop→初期化→aliveList append の第一級市民化、グループ単位のトレイル予算）。**花火トレイルの見た目に直結＝実機検証必須のため未着手。**
- **Phase 6（残り）** — `ParticleFieldData` の物理的な struct 分割（責務ごとの入れ子構造体化）。レイアウト維持＋全呼出し側波及のため、実機検証できる状況で着手するのが安全。

---

## フレーム描画パイプライン メモ（Phase 8 で調査済み）

- direct コマンドリストはフレーム先頭で開いている（`Reset` は前フレーム `PostDraw` 末尾）。
- ImGui 窓の**構築**は `MyGame::Update`（GPU コマンド発行不可）、ImGui の**GPU描画**は `MyGame::Draw` の `imGuiManager_->Draw()`（RenderDrawData）。
- `imGuiManager_->Draw()` は RT を束ね直さず、直前に束ねられた RT に描く。
- `DrawSystem::Draw` の流れ: GPUパーティクル Compute フェーズ → シャドウ → ステージループ（各ステージ offscreen へ描画＋ポストエフェクト）→ UI 合成 → `CopyFinalResultToBackBuffer`。
- パーティクル描画 PSO: RTV=R8G8B8A8_UNORM_SRGB / DSV=D24_UNORM_S8_UINT / DepthEnable=true（DepthWriteMask=ZERO）。
- RT 生成: `dxCommon->CreateRenderTextureResource(w,h,fmt,clear)`（初期状態 GENERIC_READ）＋ `srvManager->CreateSRVforRenderTexture(idx, res)`。ImGui 表示は `ImGui::Image((ImTextureID)GetGPUDescriptorHandle(idx).ptr, size)`。
- グリッドは `DrawLine3D::DrawGrid(y, division, size, color)` ＋ `DrawLine3D::Draw(vp)`（RT は束ね済み前提）。
