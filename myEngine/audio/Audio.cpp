#include "Audio.h"
#include <cassert>
#include <fstream>

Audio* Audio::instance = nullptr;

void Audio::Initialize(const std::string& directoryPath)
{
	HRESULT hr;

	directoryPath_ = directoryPath;

	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	hr = xAudio2->CreateMasteringVoice(&masterVoice);

}

Audio* Audio::GetInstance()
{
	if (instance == nullptr) {
		instance = new Audio;
	}
	return instance;
}

uint32_t Audio::LoadWave(const std::string& filename) {

	// ファイルがすでに読み込まれているかチェック
	if (loadedFiles.find(filename) != loadedFiles.end()) {
		// 既に読み込まれている場合はインデックスを返す
		for (size_t i = 0; i < kMaxSoundData; ++i) {
			if (soundDatas_[i].name_ == filename) {
				return static_cast<uint32_t>(i);
			}
		}
	}

	// ディレクトリパスとファイル名を組み合わせたフルパスを作成
	std::string fullPath = directoryPath_ + "/" + filename;

	// ファイル入力ストリームのインスタンス
	std::ifstream file;
	// .wavファイルをバイナリモードで開く
	file.open(fullPath, std::ios_base::binary);
	assert(file.is_open());

	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));

	// ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// チャンクのループを開始
	ChunkHeader chunkHeader;
	FormatChunk format = {};

	while (file.read((char*)&chunkHeader, sizeof(chunkHeader))) {
		// チャンクIDが "fmt" か確認
		if (strncmp(chunkHeader.id, "fmt ", 4) == 0) {
			// Formatチャンクのサイズを確認、データを読み込む
			assert(chunkHeader.size <= sizeof(format.fmt));

			format.chunk = chunkHeader; // チャンクヘッダーをコピー
			file.read((char*)&format.fmt, chunkHeader.size); // fmtのデータを読み込み

			break;
		}
		else {
			// 次のチャンクに移動
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}
	}

	// "fmt"チャンクが見つからなかった場合のエラーとしてだす
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// Dataチャンクの読み込み
	// Dataチャンクの読み込みとスキップ処理
	ChunkHeader data;
	while (file.read((char*)&data, sizeof(data))) {
		if (strncmp(data.id, "data", 4) == 0) {
			break; // "data" チャンクを見つけたらループを抜ける
		}
		else {
			// 現在のチャンクが "data" でない場合、そのサイズ分シーク
			file.seekg(data.size, std::ios_base::cur);
		}
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部（波形データの読み込み）
	std::vector<uint8_t> buffer(data.size);
	file.read(reinterpret_cast<char*>(buffer.data()), data.size);

	// waveファイルを閉じる
	file.close();

	// 次のインデックスにデータを格納
	SoundData& soundData = soundDatas_[soundDataIndex];
	soundData.wfex = format.fmt;
	soundData.buffer = std::move(buffer);  // std::vector のムーブ
	soundData.name_ = filename;  // ファイル名を name_ にセット

	loadedFiles.insert(filename); // 読み込んだファイル名をセットに追加

	// 現在のインデックスを返す
	uint32_t currentIndex = static_cast<uint32_t>(soundDataIndex);

	// インデックスを更新
	soundDataIndex = (soundDataIndex + 1) % kMaxSoundData;

	return currentIndex;
}



void Audio::Unload(uint32_t soundIndex)
{

	SoundData& soundData = soundDatas_[soundIndex];

	// メモリ解放は不要。vectorは自動的にメモリを管理する
	soundData.buffer.clear();  // バッファを空にする
	soundData.wfex = {};
	soundData.name_.clear();  // 名前もクリア
}

void Audio::PlayWave(uint32_t soundIndex, float volume, bool loop) {
	HRESULT result;

	const SoundData& soundData = soundDatas_[soundIndex];

	Voice* voice = new Voice(); // Voiceインスタンスを作成
	voice->handle = soundIndex; // 音声ハンドルを設定
	voice->volume = volume;     // 指定された音量を設定

	// XAudio2のコールバックインスタンスを作成
	VoiceCallback* voiceCallback = new VoiceCallback();

	// ソースボイスを作成
	result = xAudio2->CreateSourceVoice(&voice->sourceVoice, &soundData.wfex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, voiceCallback);
	assert(SUCCEEDED(result));

	// バッファを設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();  // vectorから直接バッファを取得
	buf.AudioBytes = static_cast<uint32_t>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.pContext = voice;  // コールバック用のコンテキストとしてVoiceインスタンスを渡す

	// ループ再生の設定
	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;  // 無限ループで再生
	}
	else {
		buf.LoopCount = 0;  // ループしない
	}

	// ソースボイスにバッファを送信
	result = voice->sourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	// ソースボイスを開始
	result = voice->sourceVoice->Start();
	assert(SUCCEEDED(result));

	// 音量を設定
	voice->sourceVoice->SetVolume(voice->volume);

	// 再生中のボイスをセットに追加
	voices_.insert(voice);
}

void Audio::StopWave(uint32_t soundIndex)
{

	// 再生中の音声を探す
	for (auto it = voices_.begin(); it != voices_.end(); ) {
		if ((*it)->handle == soundIndex) {
			// 音声を停止
			if ((*it)->sourceVoice != nullptr) {
				(*it)->sourceVoice->Stop(0); // 音声を停止
				(*it)->sourceVoice->DestroyVoice(); // ソースボイスを解放
			}
			delete* it; // Voiceオブジェクトを解放
			it = voices_.erase(it); // セットから削除
		}
		else {
			++it; // 次の要素へ
		}
	}
}

void Audio::SetVolume(uint32_t soundIndex, float volume)
{
	// 再生中の音声を探す
	for (auto& voice : voices_) {
		if (voice->handle == soundIndex) {
			// 音量を設定
			voice->volume = volume; // ボリュームを更新
			voice->sourceVoice->SetVolume(volume); // XAudio2のボリュームを設定
			break;
		}
	}
}

void Audio::Finalize()
{
	// マスターボイスを解放
	if (masterVoice) {
		masterVoice->DestroyVoice();
		masterVoice = nullptr;
	}

	// 再生中の音声の解放
	for (auto voice : voices_) {
		if (voice->sourceVoice) {
			// 音声の破棄を試みる
			voice->sourceVoice->DestroyVoice();

		}
		delete voice; // Voiceオブジェクトを解放
	}
	// XAudio2を解放
	if (xAudio2) {
		xAudio2.Reset();
	}

	voices_.clear(); // セットをクリア
	delete instance;
	instance = nullptr;

}

