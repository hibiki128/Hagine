#pragma once
#include"Data/DataHandler.h"
#include"Line/DrawLine3D.h"
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <Particle/ParticleCommon.h>
#include <string>
#include <type/Vector3.h>
#include <vector>
#include <wrl.h>

/// =============================================
/// フィールドの種類
/// =============================================
enum class ParticleFieldType : uint32_t {
    Wind = 0,    // 一定方向に力を加える
    Attract = 1, // 中心に引き寄せる
    Repel = 2,   // 中心から押し出す
    Vortex = 3,  // 渦巻き
};

/// =============================================
/// GPUに送るフィールドデータ (16バイトアライメント)
/// =============================================
struct ParticleFieldData {
    Vector3 position = {0, 0, 0};  // フィールドの中心座標
    float radius = 5.0f;           // 影響範囲（球）
    Vector3 direction = {1, 0, 0}; // Wind/Vortex軸方向
    float strength = 1.0f;         // 力の強さ
    uint32_t fieldType = 0;        // ParticleFieldType
    float falloff = 1.0f;          // 減衰指数（1=線形, 2=二乗）
    float padding0 = 0.0f;
    float padding1 = 0.0f;
};

/// =============================================
/// エディタ用フィールド（名前付き）
/// =============================================
struct ParticleField {
    std::string name = "NewField";
    bool enabled = true;
    ParticleFieldData data = {};
};

/// =============================================
/// ParticleFieldManager
///   シングルトン。全フィールドを管理し、
///   GPUバッファを毎フレーム更新する。
/// =============================================
class ParticleCSFieldManager {
  public:
    static ParticleCSFieldManager *GetInstance();
    static void Finalize();

    void Initialize();
    /// 毎フレーム呼ぶ：有効フィールドをGPUバッファへ転送
    void Update();

    // --- フィールド編集 ---
    void AddField(const ParticleField &field = {});
    void RemoveField(int index);
    ParticleField *GetField(int index);
    int GetFieldCount() const { return static_cast<int>(fields_.size()); }
    std::vector<ParticleField> &GetFields() { return fields_; }

    // --- フィールド生成（セーブ/ロード付き） ---
    /// フィールドを生成してシングルトンに登録し、そのポインタを返す
    /// @param name        新しいフィールドの名前（セーブファイル名にも使われる）
    /// @param templateName テンプレートとして読み込むjsonのファイル名（省略時は新規作成）
    /// @return 登録されたフィールドへのポインタ（所有権はシングルトンが持つ）
    ParticleField *CreateField(const std::string &name, const std::string &templateName = "");

    // --- セーブ/ロード ---
    /// 指定フィールドのデータをjsonに保存する
    void SaveField(const ParticleField &field);
    /// json からフィールドデータをロードして返す（失敗時は defaultField を返す）
    ParticleField LoadField(const std::string &fileName, const ParticleField &defaultField = {});

    // --- GPU ---
    /// UpdateParticle_CS の SRV に設定するハンドル
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFieldsSrvHandle() const { return fieldsSrvHandle_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetFieldCountResource() const { return fieldCountResource_; }
    uint32_t GetFieldsSrvIndex() const { return fieldsSrvIndex_; }

    // --- ImGui ---
    void DrawImGui();

    // --- ギズモ描画 ---
    /// 各フィールドの影響範囲・種類・強さを DrawLine3D でワイヤーフレーム表示する
    /// Draw() の前、毎フレーム呼ぶ
    void DrawFieldGizmos();

    static constexpr uint32_t kMaxFields = 8;

    Microsoft::WRL::ComPtr<ID3D12Resource> GetZeroFieldCountResource() const { return zeroFieldCountResource_; }

  private:
    ParticleCSFieldManager() = default;
    ~ParticleCSFieldManager() = default;
    ParticleCSFieldManager(const ParticleCSFieldManager &) = delete;
    ParticleCSFieldManager &operator=(const ParticleCSFieldManager &) = delete;

    void CreateGPUResources();
    void UploadToGPU();

    // --- セーブ/ロード内部処理 ---
    /// DataHandler を使ってフィールドデータをjsonへ書き出す
    void SaveFieldData(DataHandler &data, const ParticleField &field);
    /// DataHandler を使ってjsonからフィールドデータを読み込む
    void LoadFieldData(DataHandler &data, ParticleField &field);

    // --- ギズモ描画内部処理 ---
    /// 影響範囲球のワイヤーフレームを描画する
    void DrawFieldSphere(const ParticleField &field, const Vector4 &color);
    /// Wind フィールドの方向矢印を描画する
    void DrawWindArrows(const ParticleField &field, const Vector4 &color);
    /// Attract / Repel フィールドの放射線を描画する
    void DrawRadialLines(const ParticleField &field, const Vector4 &color, bool inward);
    /// Vortex フィールドの渦巻き円弧を描画する
    void DrawVortexArcs(const ParticleField &field, const Vector4 &color);

    static ParticleCSFieldManager *instance_;

    std::vector<ParticleField> fields_;

    // GPU側バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> fieldsResource_;
    ParticleFieldData *fieldsMappedData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> fieldCountResource_;
    uint32_t *fieldCountMappedData_ = nullptr;

    // private に追加
    Microsoft::WRL::ComPtr<ID3D12Resource> zeroFieldCountResource_;

    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> fieldsSrvHandle_{};
    uint32_t fieldsSrvIndex_ = 0;

    DirectXCommon *dxCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;
};