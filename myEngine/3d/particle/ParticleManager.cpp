#include "ParticleManager.h"
#include <random>
#include"TextureManager.h"
#include"fstream"
#include"myEngine/Frame/Frame.h"
std::unordered_map<std::string, ParticleManager::ModelData> ParticleManager::modelCache;

void ParticleManager::Initialize(SrvManager* srvManager)
{
	particleCommon = ParticleCommon::GetInstance();
	srvManager_ = srvManager;
	randomEngine.seed(seedGenerator());
}

void ParticleManager::Update(const ViewProjection& viewProjection)
{
	// ビュー射影行列の計算
	Matrix4x4 viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;

	// カメラの回転成分のみを抽出してビルボード行列を作成
	Matrix4x4 billboardMatrix = viewProjection.matView_;
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;
	billboardMatrix.m[3][3] = 1.0f;

	// ビルボード行列を逆行列にすることで、カメラに対して正しい向きにする
	billboardMatrix = Inverse(billboardMatrix);

	for (auto& [groupName, particleGroup] : particleGroups) {

		uint32_t numInstance = 0;

		// 各パーティクルの更新
		for (auto particleIterator = particleGroup.particles.begin();
			particleIterator != particleGroup.particles.end();) {

			// パーティクルの生存時間チェック
			if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
				particleIterator = particleGroup.particles.erase(particleIterator);
				continue;
			}
			// パーティクルの生存時間に基づく進行度 t を計算
			float t = (*particleIterator).currentTime / (*particleIterator).lifeTime;
			t = std::clamp(t, 0.0f, 1.0f);

			// 拡縮処理
			// 拡縮処理
			if (isSinMove_) {

				// Sin波の周波数制御 (速度調整)
				float waveScale = 0.5f * (sin(t * DirectX::XM_PI * 18.0f) + 1.0f);  // 0 ～ 1 の範囲

				// 最大スケールが寿命に応じて縮小し、最終的に0になる
				float maxScale = (1.0f - t);  // 1 -> 0 に線形に縮小

				// Sin波スケールと最大スケールの積を適用
				(*particleIterator).transform.scale_ =
					(*particleIterator).startScale * waveScale * maxScale;
			}

			else {
				// 通常の線形補間
				(*particleIterator).transform.scale_ =
					(1.0f - t) * (*particleIterator).startScale + t * (*particleIterator).endScale;
				// アルファ値の計算
				(*particleIterator).color.w = (*particleIterator).initialAlpha - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
			}

			(*particleIterator).Acce = (1.0f - t) * (*particleIterator).startAcce + t * (*particleIterator).endAcce;
			if (isFaceDirection_) {
				// 固定された進行方向を使用して回転を設定
				Vector3 forward = (*particleIterator).fixedDirection; // 初期に保存された進行方向
				Vector3 initialUp = { 0.0f, 1.0f, 0.0f };

				// 回転軸を計算
				Vector3 rotationAxis = initialUp.Cross(forward).Normalize();
				float dotProduct = initialUp.Dot(forward);
				float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f)); // 安全な範囲にクランプ

				// 回転を設定
				(*particleIterator).transform.rotation_.x = rotationAxis.x * angle;
				(*particleIterator).transform.rotation_.y = rotationAxis.y * angle;
				(*particleIterator).transform.rotation_.z = rotationAxis.z * angle;

			}
			else if (isRandomRotate_) {
				// ランダム回転の場合の処理
				(*particleIterator).transform.rotation_ += (*particleIterator).rotateVelocity;
			}
			else {
				// 通常の回転補間
				(*particleIterator).transform.rotation_ =
					(1.0f - t) * (*particleIterator).startRote + t * (*particleIterator).endRote;
			}
			if (isAcceMultipy_) {
				(*particleIterator).velocity *= (*particleIterator).Acce;
			}
			else {
				(*particleIterator).velocity += (*particleIterator).Acce;
			}
			// パーティクルの移動
			(*particleIterator).transform.translation_ +=
				(*particleIterator).velocity * Frame::DeltaTime();
			(*particleIterator).currentTime += Frame::DeltaTime();

			// ワールド行列の計算
			Matrix4x4 worldMatrix{};
			if (isBillboard) {
				// ビルボード処理の場合、カメラ方向に向ける
				worldMatrix = MakeScaleMatrix((*particleIterator).transform.scale_) * billboardMatrix *
					MakeTranslateMatrix((*particleIterator).transform.translation_);
			}
			else {
				// 通常のアフィン変換行列を使用
				worldMatrix = MakeAffineMatrix((*particleIterator).transform.scale_,
					(*particleIterator).transform.rotation_,
					(*particleIterator).transform.translation_);
			}

			// パーティクルのワールド位置をチェック
			WorldTransform particleWorldTransform;
			particleWorldTransform.translation_ = (*particleIterator).transform.translation_;

			//// 視野外判定
			//if (viewProjection.IsOutsideViewFrustum(particleWorldTransform)) {
			//	// 視野外であれば、パーティクルを削除
			//	particleIterator = particleGroup.particles.erase(particleIterator);
			//	continue;
			//}

			// ワールド・ビュー・プロジェクション行列の計算
			Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

			// インスタンスデータに設定
			if (numInstance < kNumMaxInstance) {
				particleGroup.instancingData[numInstance].WVP = worldViewProjectionMatrix;
				particleGroup.instancingData[numInstance].World = worldMatrix;
				particleGroup.instancingData[numInstance].color = (*particleIterator).color;
				particleGroup.instancingData[numInstance].color.w = (*particleIterator).color.w;  // アルファ値の設定
				++numInstance;
			}

			++particleIterator;
		}

		// インスタンス数の更新
		particleGroup.instanceCount = numInstance;
	}
}

void ParticleManager::Draw()
{

	particleCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	for (auto& [groupName, particleGroup] : particleGroups) {
		if (particleGroup.instanceCount > 0) {
			particleCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			srvManager_->SetGraphicsRootDescriptorTable(1, particleGroup.instancingSRVIndex);

			srvManager_->SetGraphicsRootDescriptorTable(2, modelData.material.textureIndex);

			particleCommon->GetDxCommon()->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), particleGroup.instanceCount, 0, 0);
		}
	}
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string& filename)
{
	if (particleGroups.contains(name)) {
		return;
	}

	particleGroups[name] = ParticleGroup();
	ParticleGroup& particleGroup = particleGroups[name];
	CreateVartexData(filename);
	particleGroup.material.textureFilePath = modelData.material.textureFilePath;
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
	particleGroup.material.textureIndex = modelData.material.textureIndex;
	particleGroup.instancingResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	particleGroup.instancingSRVIndex = srvManager_->Allocate() + 1;
	particleGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&particleGroup.instancingData));

	srvManager_->CreateSRVforStructuredBuffer(particleGroup.instancingSRVIndex, particleGroup.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

	CreateMaterial();
	particleGroup.instanceCount = 0;
}

void ParticleManager::SetTexture(const std::string& filePath)
{
	TextureManager::GetInstance()->LoadTexture(filePath);
	modelData.material.textureFilePath = filePath;
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
}

void ParticleManager::CreateVartexData(const std::string& filename)
{
	modelData = LoadObjFile("resources/models/", filename);

	// 頂点リソースを作る
	vertexResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファビューを作成する
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();	// リソースの先頭アドレスから使う
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());		// 使用するリソースのサイズは頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);	// 1頂点当たりのサイズ

	// 頂点リソースにデータを書き込む
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));		// 書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

ParticleManager::Particle ParticleManager::MakeNewParticle(
	std::mt19937& randomEngine,
	const Vector3& translate,
	const Vector3& rotation,
	const Vector3& scale, // スケールを引数として受け取る
	const Vector3& velocityMin, const Vector3& velocityMax, // 速度の範囲
	float lifeTimeMin, float lifeTimeMax,
	const Vector3& particleStartScale, const Vector3& particleEndScale,
	const Vector3& startAcce, const Vector3& endAcce,
	const Vector3& startRote, const Vector3& endRote,
	bool isRamdomColor, float alphaMin, float alphaMax,
	const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
	const Vector3& allScaleMax, const Vector3& allScaleMin,
	const float& scaleMin, const float& scaleMax
)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distVelocityX(velocityMin.x, velocityMax.x);
	std::uniform_real_distribution<float> distVelocityY(velocityMin.y, velocityMax.y);
	std::uniform_real_distribution<float> distVelocityZ(velocityMin.z, velocityMax.z);
	std::uniform_real_distribution<float> distLifeTime(lifeTimeMin, lifeTimeMax);
	std::uniform_real_distribution<float> distAlpha(alphaMin, alphaMax);

	Particle particle;

	// スケールを考慮したランダムな位置を生成
	Vector3 randomTranslate = {
		distribution(randomEngine) * scale.x,
		distribution(randomEngine) * scale.y,
		distribution(randomEngine) * scale.z
	};

	// 回転行列を適用してランダムな位置を回転
	Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(rotation);

	// ランダム位置を回転行列で変換
	Vector3 rotatedPosition = {
		randomTranslate.x * rotationMatrix.m[0][0] + randomTranslate.y * rotationMatrix.m[1][0] + randomTranslate.z * rotationMatrix.m[2][0],
		randomTranslate.x * rotationMatrix.m[0][1] + randomTranslate.y * rotationMatrix.m[1][1] + randomTranslate.z * rotationMatrix.m[2][1],
		randomTranslate.x * rotationMatrix.m[0][2] + randomTranslate.y * rotationMatrix.m[1][2] + randomTranslate.z * rotationMatrix.m[2][2]
	};

	// 回転されたランダムな位置をトランスレーションに加算
	particle.transform.translation_ = translate + rotatedPosition;

	if (isRandomAllSize_) {
		std::uniform_real_distribution<float> distScaleX(allScaleMin.x, allScaleMax.x);
		std::uniform_real_distribution<float> distScaleY(allScaleMin.y, allScaleMax.y);
		std::uniform_real_distribution<float> distScaleZ(allScaleMin.z, allScaleMax.z);
		particle.startScale = { distScaleX(randomEngine),distScaleY(randomEngine),distScaleZ(randomEngine) };
	}
	else if (isRandomSize_) {
		std::uniform_real_distribution<float> distScale(scaleMin, scaleMax);
		particle.startScale.x = distScale(randomEngine);
		particle.startScale.y = particle.startScale.x;
		particle.startScale.z = particle.startScale.x;
	}
	else {
		particle.startScale = particleStartScale;
	}
	particle.endScale = particleEndScale;

	particle.startAcce = startAcce;
	particle.endAcce = endAcce;

	// パーティクルの速度をランダムに設定
	Vector3 randomVelocity = {
		distVelocityX(randomEngine),
		distVelocityY(randomEngine),
		distVelocityZ(randomEngine)
	};

	// エミッターの回転を速度ベクトルに適用
	particle.velocity = {
		randomVelocity.x * rotationMatrix.m[0][0] + randomVelocity.y * rotationMatrix.m[1][0] + randomVelocity.z * rotationMatrix.m[2][0],
		randomVelocity.x * rotationMatrix.m[0][1] + randomVelocity.y * rotationMatrix.m[1][1] + randomVelocity.z * rotationMatrix.m[2][1],
		randomVelocity.x * rotationMatrix.m[0][2] + randomVelocity.y * rotationMatrix.m[1][2] + randomVelocity.z * rotationMatrix.m[2][2]
	};


	if (isRandomRotate_) {
		// 回転速度をランダムに設定
		std::uniform_real_distribution<float> distRotateXVelocity(rotateVelocityMin.x, rotateVelocityMax.x);
		std::uniform_real_distribution<float> distRotateYVelocity(rotateVelocityMin.y, rotateVelocityMax.y);
		std::uniform_real_distribution<float> distRotateZVelocity(rotateVelocityMin.z, rotateVelocityMax.z);
		std::uniform_real_distribution<float> distRotateX(0.0f, 2.0f);
		std::uniform_real_distribution<float> distRotateY(0.0f, 2.0f);
		std::uniform_real_distribution<float> distRotateZ(0.0f, 2.0f);
		particle.rotateVelocity.x = distRotateXVelocity(randomEngine);
		particle.rotateVelocity.y = distRotateYVelocity(randomEngine);
		particle.rotateVelocity.z = distRotateZVelocity(randomEngine);
		particle.transform.rotation_.x = distRotateX(randomEngine);
		particle.transform.rotation_.y = distRotateY(randomEngine);
		particle.transform.rotation_.z = distRotateZ(randomEngine);
	}
	else {
		particle.startRote = startRote;
		particle.endRote = endRote;
	}

	if (isRamdomColor) {
		// ランダムな色を設定
		std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
		particle.color = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), distAlpha(randomEngine) };
	}
	else {
		particle.color = { 1.0f,1.0f,1.0f, distAlpha(randomEngine) };
	}

	// 初期向きは上方向（0, 1, 0）
	Vector3 initialUp = { 0.0f, 1.0f, 0.0f };

	// パーティクルの進行方向（velocity）を正規化
	Vector3 forward = particle.velocity.Normalize();

	// 固定された進行方向として保存
	particle.fixedDirection = forward;

	// 回転軸を計算（初期向きと進行方向の外積）
	Vector3 rotationAxis = initialUp.Cross(forward).Normalize();

	// 初期向きと進行方向のなす角を計算
	float dotProduct = initialUp.Dot(forward);
	float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f)); // 安全のためclamp

	// 回転軸と角度から回転を生成
	particle.transform.rotation_.x = rotationAxis.x * angle; // 軸回転の簡易的な設定
	particle.transform.rotation_.y = rotationAxis.y * angle;
	particle.transform.rotation_.z = rotationAxis.z * angle;


	particle.initialAlpha = distAlpha(randomEngine);
	// ライフタイムをランダムに設定
	particle.lifeTime = distLifeTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

ParticleManager::MaterialData ParticleManager::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // 開けなかったら止める
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath =  textureFilename;
		}
	}

	// テクスチャが張られていない場合の処理
	if (materialData.textureFilePath.empty()) {
		materialData.textureFilePath =  "debug/white1x1.png";
	}

	return materialData;
}


ParticleManager::ModelData ParticleManager::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	std::string fullPath = directoryPath + filename;

	// キャッシュを確認して、既に読み込まれている場合はそれを返す
	auto it = modelCache.find(fullPath);
	if (it != modelCache.end()) {
		return it->second;
	}

	ModelData modelData;
	std::vector<Vector4> positions; // 位置
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ1行目を格納するもの

	// ファイル名からフォルダ部分を取得
	std::string folderPath;
	size_t lastSlashPos = filename.find_last_of("/\\");
	if (lastSlashPos != std::string::npos) {
		folderPath = filename.substr(0, lastSlashPos);
	}

	std::ifstream file(fullPath); // ファイルを開く
	assert(file.is_open()); // ファイルが開けなかったら停止

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				VertexData vertex = { position, texcoord };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position, texcoord };
			}
		}
		else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			if (!folderPath.empty()) {
				modelData.material = LoadMaterialTemplateFile(directoryPath + folderPath, materialFilename);
			}
			else {
				modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
			}
		}
	}

	// キャッシュに保存
	modelCache[fullPath] = modelData;

	return modelData;
}


void ParticleManager::CreateMaterial()
{
	// Sprite用のマテリアルリソースをつくる
	materialResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 色の設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// Lightingの設定
	materialData->uvTransform = MakeIdentity4x4();
}

std::list<ParticleManager::Particle> ParticleManager::Emit(
	const std::string name,
	const Vector3& position,
	uint32_t count,
	const Vector3& scale, // スケールを引数として追加
	const Vector3& velocityMin, const Vector3& velocityMax, // 速度の範囲を引数として追加
	float lifeTimeMin, float lifeTimeMax,
	const Vector3& particleStartScale, const Vector3& particleEndScale,
	const Vector3& startAcce, const Vector3& endAcce,
	const Vector3& startRote, const Vector3& endRote,
	bool isRandomColor, float alphaMin, float alphaMax,
	const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
	const Vector3& allScaleMax, const Vector3& allScaleMin,
	const float& scaleMin, const float& scaleMax, const Vector3& rotation)
{
	// パーティクルグループが存在するか確認
	assert(particleGroups.find(name) != particleGroups.end() && "Error: パーティクルグループが存在しません。");

	// 指定されたパーティクルグループを取得
	ParticleGroup& particleGroup = particleGroups[name];

	// 新しいパーティクルを生成し、パーティクルグループに追加
	std::list<Particle> newParticles;
	for (uint32_t nowCount = 0; nowCount < count; ++nowCount) {
		// 新たなパーティクルを作成
		Particle particle = MakeNewParticle(
			randomEngine,
			position,
			rotation,
			scale,          // 追加されたスケール
			velocityMin,
			velocityMax,    // 追加された速度の範囲
			lifeTimeMin,
			lifeTimeMax,    // 追加されたライフタイムの範囲
			particleStartScale,
			particleEndScale,
			startAcce,
			endAcce,
			startRote,
			endRote,
			isRandomColor,
			alphaMin,
			alphaMax,
			rotateVelocityMin,
			rotateVelocityMax,
			allScaleMax, allScaleMin,
			scaleMin, scaleMax
		);
		newParticles.push_back(particle);
	}

	// 新たに生成されたパーティクルをグループに追加
	particleGroup.particles.splice(particleGroup.particles.end(), newParticles);

	return newParticles; // 作成されたパーティクルを返す
}
