#include "GpuProfiler.h"
#include <DirectXCommon.h>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace Hagine {

GpuProfiler *GpuProfiler::GetInstance() {
    static GpuProfiler instance;
    return &instance;
}

void GpuProfiler::EnsureInit() {
    if (initialized_)
        return;

    dxCommon_ = DirectXCommon::GetInstance();
    if (!dxCommon_ || !dxCommon_->GetDevice())
        return;

    ID3D12Device *device = dxCommon_->GetDevice().Get();

    // タイムスタンプ用 QueryHeap（kRing フレーム分）
    D3D12_QUERY_HEAP_DESC qhd{};
    qhd.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    qhd.Count = kTotalSlots;
    qhd.NodeMask = 0;
    if (FAILED(device->CreateQueryHeap(&qhd, IID_PPV_ARGS(&queryHeap_))))
        return;

    // Readback バッファ（resolve 先・CPU 読み取り）
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = static_cast<UINT64>(kTotalSlots) * sizeof(uint64_t);
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&readback_))))
        return;
    readback_->SetName(L"GpuProfiler_Readback");
    readback_->Map(0, nullptr, reinterpret_cast<void **>(&mapped_));

    // キューごとのタイムスタンプ周波数（tick/秒）。Direct と Compute で異なりうる。
    if (auto *gq = dxCommon_->GetCommandQueue())
        gq->GetTimestampFrequency(&freqGraphics_);
    if (auto *cq = dxCommon_->GetComputeCommandQueue())
        cq->GetTimestampFrequency(&freqCompute_);

    initialized_ = true;
}

void GpuProfiler::BeginFrame() {
    EnsureInit();
    if (!initialized_)
        return;

    // ring を進める。これから使う ring には kRing フレーム前の結果が入っており、
    // GPU は既に完了している（in-flight は最大2フレーム）ので安全に読み戻せる。
    ringIndex_ = (ringIndex_ + 1) % kRing;
    pairCursor_ = 0;
    RingFrame &rf = rings_[ringIndex_];

    if (rf.valid && mapped_) {
        results_.clear();
        const uint32_t ringBase = ringIndex_ * kSlotsPerRing;
        for (const auto &e : rf.entries) {
            uint64_t t0 = mapped_[ringBase + e.pair * 2];
            uint64_t t1 = mapped_[ringBase + e.pair * 2 + 1];
            uint64_t freq = e.isCompute ? freqCompute_ : freqGraphics_;
            double ms = (freq > 0 && t1 >= t0) ? static_cast<double>(t1 - t0) * 1000.0 / static_cast<double>(freq) : 0.0;

            // 同ラベル（複数エミッターの同名パス）は合算する
            bool merged = false;
            for (auto &r : results_) {
                if (r.isCompute == e.isCompute && r.label == e.label) {
                    r.ms += ms;
                    merged = true;
                    break;
                }
            }
            if (!merged)
                results_.push_back({e.label, ms, e.isCompute});
        }
    }

    // このフレームの記録用にクリア。今フレームの resolve 後、kRing フレーム後に読み戻される。
    rf.entries.clear();
    rf.valid = true;
}

int GpuProfiler::Open(ID3D12GraphicsCommandList *cl, const char *label, bool isCompute) {
    if (!enabled_ || !initialized_ || !cl)
        return -1;
    if (pairCursor_ >= kMaxPairsPerFrame)
        return -1;

    uint32_t pair = pairCursor_++;
    uint32_t slot = ringIndex_ * kSlotsPerRing + pair * 2;
    cl->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slot);

    RingFrame &rf = rings_[ringIndex_];
    int handle = static_cast<int>(rf.entries.size());
    rf.entries.push_back({label ? label : "?", pair, isCompute});
    return handle;
}

int GpuProfiler::OpenCompute(ID3D12GraphicsCommandList *cl, const char *label) {
    return Open(cl, label, true);
}

int GpuProfiler::OpenGraphics(ID3D12GraphicsCommandList *cl, const char *label) {
    return Open(cl, label, false);
}

void GpuProfiler::Close(ID3D12GraphicsCommandList *cl, int handle) {
    if (!enabled_ || !initialized_ || !cl || handle < 0)
        return;
    RingFrame &rf = rings_[ringIndex_];
    if (handle >= static_cast<int>(rf.entries.size()))
        return;
    uint32_t pair = rf.entries[handle].pair;
    uint32_t slot = ringIndex_ * kSlotsPerRing + pair * 2 + 1;
    cl->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slot);
}

void GpuProfiler::Resolve(ID3D12GraphicsCommandList *cl, bool isCompute) {
    if (!enabled_ || !initialized_ || !cl)
        return;
    RingFrame &rf = rings_[ringIndex_];
    const uint32_t ringBase = ringIndex_ * kSlotsPerRing;
    for (const auto &e : rf.entries) {
        if (e.isCompute != isCompute)
            continue;
        uint32_t startSlot = ringBase + e.pair * 2;
        cl->ResolveQueryData(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
                             startSlot, 2, readback_.Get(),
                             static_cast<UINT64>(startSlot) * sizeof(uint64_t));
    }
}

void GpuProfiler::ResolveCompute(ID3D12GraphicsCommandList *cl) { Resolve(cl, true); }
void GpuProfiler::ResolveGraphics(ID3D12GraphicsCommandList *cl) { Resolve(cl, false); }

void GpuProfiler::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("GPU プロファイラ (パス別)")) {
        bool en = enabled_;
        if (ImGui::Checkbox("計測ON##gpuprof", &en))
            enabled_ = en;
        ImGui::SameLine();
        ImGui::TextDisabled("(3F遅延)");

        double computeTotal = 0.0, graphicsTotal = 0.0;
        for (const auto &r : results_) {
            (r.isCompute ? computeTotal : graphicsTotal) += r.ms;
        }

        if (ImGui::BeginTable("##gpuprofTable", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("キュー", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("パス", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);

            for (const auto &r : results_) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(r.isCompute ? "Compute" : "Graphics");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(r.label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", r.ms);
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Text("Compute 合計: %.3f ms", computeTotal);
        ImGui::Text("Graphics 合計: %.3f ms", graphicsTotal);
        ImGui::Text("GPU 合計: %.3f ms", computeTotal + graphicsTotal);
    }
#endif
}
} // namespace Hagine
