#include "Audio.h"
#include <cassert>
#include <fstream>

#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

void Audio::Initialize(const std::string &directoryPath) {
    HRESULT hr;

    directoryPath_ = directoryPath;

    hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    hr = xAudio2->CreateMasteringVoice(&masterVoice);
}

uint32_t Audio::LoadWave(const std::string &filename) {

    if (loadedFiles.find(filename) != loadedFiles.end()) {
        for (size_t i = 0; i < kMaxSoundData; ++i) {
            if (soundDatas_[i].name_ == filename) {
                return static_cast<uint32_t>(i);
            }
        }
    }

    std::string fullPath = directoryPath_ + "/" + filename;

    std::ifstream file;
    file.open(fullPath, std::ios_base::binary);
    assert(file.is_open());

    RiffHeader riff;
    file.read((char *)&riff, sizeof(riff));

    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
        assert(0);
    }
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0);
    }

    ChunkHeader chunkHeader;
    FormatChunk format = {};

    while (file.read((char *)&chunkHeader, sizeof(chunkHeader))) {
        if (strncmp(chunkHeader.id, "fmt ", 4) == 0) {
            assert(chunkHeader.size <= sizeof(format.fmt));
            format.chunk = chunkHeader;
            file.read((char *)&format.fmt, chunkHeader.size);
            break;
        } else {
            file.seekg(chunkHeader.size, std::ios_base::cur);
        }
    }

    if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
        assert(0);
    }

    ChunkHeader data;
    while (file.read((char *)&data, sizeof(data))) {
        if (strncmp(data.id, "data", 4) == 0) {
            break;
        } else {
            file.seekg(data.size, std::ios_base::cur);
        }
    }

    if (strncmp(data.id, "data", 4) != 0) {
        assert(0);
    }

    std::vector<uint8_t> buffer(data.size);
    file.read(reinterpret_cast<char *>(buffer.data()), data.size);
    file.close();

    SoundData &soundData = soundDatas_[soundDataIndex];
    soundData.wfex = format.fmt;
    soundData.buffer = std::move(buffer);
    soundData.name_ = filename;

    loadedFiles.insert(filename);

    uint32_t currentIndex = static_cast<uint32_t>(soundDataIndex);
    soundDataIndex = (soundDataIndex + 1) % kMaxSoundData;

    return currentIndex;
}

void Audio::Unload(uint32_t soundIndex) {
    SoundData &soundData = soundDatas_[soundIndex];
    soundData.buffer.clear();
    soundData.wfex = {};
    soundData.name_.clear();
}

void Audio::PlayWave(uint32_t soundIndex, float volume, bool loop) {
    HRESULT result;

    const SoundData &soundData = soundDatas_[soundIndex];

    Voice *voice = new Voice();
    voice->handle = soundIndex;
    voice->volume = volume;

    VoiceCallback *voiceCallback = new VoiceCallback();

    result = xAudio2->CreateSourceVoice(&voice->sourceVoice, &soundData.wfex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, voiceCallback);
    assert(SUCCEEDED(result));

    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.buffer.data();
    buf.AudioBytes = static_cast<uint32_t>(soundData.buffer.size());
    buf.Flags = XAUDIO2_END_OF_STREAM;
    buf.pContext = voice;
    buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    result = voice->sourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(result));

    result = voice->sourceVoice->Start();
    assert(SUCCEEDED(result));

    voice->sourceVoice->SetVolume(voice->volume);

    voices_.insert(voice);
}

void Audio::StopWave(uint32_t soundIndex) {
    for (auto it = voices_.begin(); it != voices_.end();) {
        if ((*it)->handle == soundIndex) {
            if ((*it)->sourceVoice != nullptr) {
                (*it)->sourceVoice->Stop(0);
                (*it)->sourceVoice->DestroyVoice();
            }
            delete *it;
            it = voices_.erase(it);
        } else {
            ++it;
        }
    }
}

void Audio::SetVolume(uint32_t soundIndex, float volume) {
    for (auto &voice : voices_) {
        if (voice->handle == soundIndex) {
            voice->volume = volume;
            voice->sourceVoice->SetVolume(volume);
            break;
        }
    }
}

void Audio::Finalize() {
    if (masterVoice) {
        masterVoice->DestroyVoice();
        masterVoice = nullptr;
    }

    for (auto voice : voices_) {
        if (voice->sourceVoice) {
            voice->sourceVoice->DestroyVoice();
        }
        delete voice;
    }

    if (xAudio2) {
        xAudio2.Reset();
    }

    voices_.clear();
}

//==============================================================================
// デバッグ補助関数
//==============================================================================

void Audio::DebugScanWavFiles() {
    debugWavFileList_.clear();

    // ソリューション直下の Resources\sounds\ を走査
    // 実行ファイルがソリューション直下に置かれている前提
    std::filesystem::path dir("Resources/sounds");

    if (!std::filesystem::exists(dir)) {
        return;
    }

    for (auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".wav") {
            continue;
        }

        // directoryPath_ からの相対パスをファイル名として登録
        auto rel = std::filesystem::relative(entry.path(), dir);
        debugWavFileList_.push_back(rel.string());
    }
}

float Audio::DebugGetDurationSec(uint32_t index) const {
    const SoundData &sd = soundDatas_[index];
    if (sd.wfex.nAvgBytesPerSec == 0) {
        return 0.0f;
    }
    return static_cast<float>(sd.buffer.size()) /
           static_cast<float>(sd.wfex.nAvgBytesPerSec);
}

float Audio::DebugGetPositionSec(uint32_t index) const {
    for (auto *voice : voices_) {
        if (voice->handle != index) {
            continue;
        }
        if (!voice->sourceVoice) {
            continue;
        }

        XAUDIO2_VOICE_STATE state{};
        voice->sourceVoice->GetState(&state);

        const SoundData &sd = soundDatas_[index];
        if (sd.wfex.nAvgBytesPerSec == 0) {
            return 0.0f;
        }

        // SamplesPlayed をバイト換算して秒に変換
        uint64_t bytes = state.SamplesPlayed *
                         sd.wfex.nBlockAlign;
        return static_cast<float>(bytes) /
               static_cast<float>(sd.wfex.nAvgBytesPerSec);
    }
    return 0.0f;
}

bool Audio::DebugIsPlaying(uint32_t index) const {
    for (auto *voice : voices_) {
        if (voice->handle == index) {
            return true;
        }
    }
    return false;
}

uint32_t Audio::DebugResolveIndex(const std::string &filename) const {
    auto it = debugLoadedMap_.find(filename);
    if (it != debugLoadedMap_.end()) {
        return it->second;
    }
    return UINT32_MAX;
}

//==============================================================================
// Debug() 本体
//==============================================================================

void Audio::Debug() {
#ifdef _DEBUG
    //------------------------------------------------------------------
    // マスター音量
    //------------------------------------------------------------------
    ImGui::SeparatorText("Master");
    if (ImGui::SliderFloat("Master Volume", &debugMasterVolume_, 0.0f, 1.0f)) {
        if (masterVoice) {
            masterVoice->SetVolume(debugMasterVolume_);
        }
    }

    //------------------------------------------------------------------
    // ファイルスキャン
    //------------------------------------------------------------------
    ImGui::SeparatorText("File Browser  [ Resources\\sounds\\ ]");

    if (ImGui::Button("Scan .wav Files")) {
        DebugScanWavFiles();
        debugSelectedFile_ = -1;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(%zu files found)", debugWavFileList_.size());

    // ファイルリスト
    ImGui::BeginChild("##filelist", ImVec2(0, 160), true);
    for (int i = 0; i < static_cast<int>(debugWavFileList_.size()); ++i) {
        const bool selected = (debugSelectedFile_ == i);
        if (ImGui::Selectable(debugWavFileList_[i].c_str(), selected)) {
            debugSelectedFile_ = i;
        }
    }
    ImGui::EndChild();

    //------------------------------------------------------------------
    // 選択ファイルの操作
    //------------------------------------------------------------------
    ImGui::SeparatorText("Playback Control");

    const bool hasSelection = (debugSelectedFile_ >= 0 &&
                               debugSelectedFile_ < static_cast<int>(debugWavFileList_.size()));

    if (!hasSelection) {
        ImGui::TextDisabled("-- Select a file above --");
    } else {
        const std::string &selectedName = debugWavFileList_[debugSelectedFile_];
        ImGui::Text("File : %s", selectedName.c_str());

        uint32_t idx = DebugResolveIndex(selectedName);
        const bool loaded = (idx != UINT32_MAX);
        const bool playing = loaded && DebugIsPlaying(idx);

        // ロード状態バッジ
        if (loaded) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[Loaded  idx=%u]", idx);
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[Not Loaded]");
        }

        // 再生パラメータ
        ImGui::SliderFloat("Volume", &debugVolume_, 0.0f, 1.0f);
        ImGui::Checkbox("Loop", &debugLoop_);

        ImGui::Spacing();

        //-- Load ボタン
        if (!loaded) {
            if (ImGui::Button("Load")) {
                // Resources\sounds\ を directoryPath_ として LoadWave へ渡す
                // directoryPath_ が既に "Resources/sounds" 相当に設定されている想定
                // ただしデバッグ用に固定パスからロード
                std::string savedDir = directoryPath_;
                directoryPath_ = "Resources/sounds";
                uint32_t newIdx = LoadWave(selectedName);
                directoryPath_ = savedDir;
                debugLoadedMap_[selectedName] = newIdx;
            }
        } else {
            //-- Play ボタン
            ImGui::BeginDisabled(playing);
            if (ImGui::Button("Play")) {
                PlayWave(idx, debugVolume_, debugLoop_);
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            //-- Stop ボタン
            ImGui::BeginDisabled(!playing);
            if (ImGui::Button("Stop")) {
                StopWave(idx);
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            //-- 再生中のみ音量を即時反映
            if (playing) {
                if (ImGui::Button("Apply Volume")) {
                    SetVolume(idx, debugVolume_);
                }
            }

            ImGui::SameLine();

            //-- Unload ボタン
            ImGui::BeginDisabled(playing);
            if (ImGui::Button("Unload")) {
                StopWave(idx);
                Unload(idx);
                debugLoadedMap_.erase(selectedName);
                loadedFiles.erase(selectedName);
            }
            ImGui::EndDisabled();

            //-- 再生時間バー
            if (loaded) {
                float duration = DebugGetDurationSec(idx);
                float position = playing ? DebugGetPositionSec(idx) : 0.0f;

                // ループ時は position が duration を超えることがあるのでクランプ
                if (duration > 0.0f) {
                    position = (position > duration) ? std::fmod(position, duration) : position;
                }

                ImGui::Spacing();

                // プログレスバー
                float fraction = (duration > 0.0f) ? (position / duration) : 0.0f;
                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%.2f s / %.2f s", position, duration);
                ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);

                // 詳細情報
                if (duration > 0.0f) {
                    const SoundData &sd = soundDatas_[idx];
                    ImGui::TextDisabled(
                        "Ch:%u  Rate:%u Hz  Bits:%u  Avg:%u B/s",
                        sd.wfex.nChannels,
                        sd.wfex.nSamplesPerSec,
                        sd.wfex.wBitsPerSample,
                        sd.wfex.nAvgBytesPerSec);
                }
            }
        }
    }

    //------------------------------------------------------------------
    // 全ロード済みファイル一覧
    //------------------------------------------------------------------
    ImGui::SeparatorText("Loaded Files");
    ImGui::BeginChild("##loadedlist", ImVec2(0, 120), true);
    for (auto &[name, soundIdx] : debugLoadedMap_) {
        bool isPlaying = DebugIsPlaying(soundIdx);
        float dur = DebugGetDurationSec(soundIdx);
        float pos = isPlaying ? DebugGetPositionSec(soundIdx) : 0.0f;

        if (isPlaying) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[PLAY]");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[STOP]");
        }
        ImGui::SameLine();
        ImGui::Text("idx=%-4u  %.2f/%.2fs  %s", soundIdx, pos, dur, name.c_str());
    }
    ImGui::EndChild();

    //------------------------------------------------------------------
    // 全停止ボタン
    //------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Stop All", ImVec2(-1.0f, 0.0f))) {
        for (auto &[name, soundIdx] : debugLoadedMap_) {
            StopWave(soundIdx);
        }
    }
    ImGui::PopStyleColor(2);
#endif // _DEBUG
}