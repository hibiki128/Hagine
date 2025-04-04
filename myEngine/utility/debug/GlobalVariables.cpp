#include "GlobalVariables.h"
#include "Windows.h"
#include "fstream"
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG


void GlobalVariables::Update() {
#ifdef _DEBUG

	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	// タブバーの開始
	if (ImGui::BeginTabBar("GlobalVariablesTabBar")) {
		// 各グループについてタブを作成
		for (std::map<std::string, Group>::iterator itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {
			// グループ名を取得
			const std::string& groupName = itGroup->first;
			// グループの参照を取得
			Group& group = itGroup->second;

			// タブの開始
			if (ImGui::BeginTabItem(groupName.c_str())) {

				// タブが選択されているときにその内容を表示
				// 各項目について
				for (std::map<std::string, Item>::iterator itItem = group.items.begin(); itItem != group.items.end(); ++itItem) {
					// 項目名を取得
					const std::string& itemName = itItem->first;
					// 項目の参照を取得
					Item& item = itItem->second;

					// int32_t型の値を保持していれば
					if (std::holds_alternative<int32_t>(item.value)) {
						int32_t* ptr = std::get_if<int32_t>(&item.value);
						ImGui::DragInt(itemName.c_str(), ptr, 1);
					} // float型の値を保持していれば
					else if (std::holds_alternative<float>(item.value)) {
						float* ptr = std::get_if<float>(&item.value);
						ImGui::DragFloat(itemName.c_str(), ptr, 0.1f);
					} // Vector2型の値を保持していれば
					else if (std::holds_alternative<Vector2>(item.value)) {
						Vector2* ptr = std::get_if<Vector2>(&item.value);
						ImGui::DragFloat2(itemName.c_str(), reinterpret_cast<float*>(ptr), 0.1f);
					} // Vector3型の値を保持していれば
					else if (std::holds_alternative<Vector3>(item.value)) {
						Vector3* ptr = std::get_if<Vector3>(&item.value);
						ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), 0.1f);
					} // bool型の値を保持していれば
					else if (std::holds_alternative<bool>(item.value)) {
						bool* ptr = std::get_if<bool>(&item.value);
						ImGui::Checkbox(itemName.c_str(), ptr);
					}
				}

				// 改行
				ImGui::Text("\n");

				// 保存ボタン
				if (ImGui::Button("Save")) {
					SaveFile(groupName);
					std::string message = std::format("{}.json saved.", groupName);
					MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
				}

				ImGui::EndTabItem(); // タブの終了
			}
		}
		ImGui::EndTabBar(); // タブバーの終了
	}

	ImGui::End(); // Global Variables ウィンドウの終了
#endif // _DEBUG
}


GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::CreateGroup(const std::string& groupName) {
	// 指定名のオブジェクトがなければ追加する
	datas_[groupName];
}

void GlobalVariables::SaveFile(const std::string& groupName) {
	// グループを検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());

	json root;

	root = json::object();

	// jsonオブジェクト登録
	root[groupName] = json::object();

	// 各項目について
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.items.begin(); itItem != itGroup->second.items.end(); ++itItem) {
		// 各項目名を取得
		const std::string& itemName = itItem->first;
		// 項目の参照を取得
		Item& item = itItem->second;

		// int32_t型の値を保持していれば
		if (std::holds_alternative<int32_t>(item.value)) {
			// int32_t型の値を登録
			root[groupName][itemName] = std::get<int32_t>(item.value);
		} // float型の値を保持していれば
		else if (std::holds_alternative<float>(item.value)) {
			// float型の値を登録
			root[groupName][itemName] = std::get<float>(item.value);
		}
		else if (std::holds_alternative<Vector2>(item.value)) {
			// float型のjson配列登録
			Vector2 value = std::get<Vector2>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y });
		} // Vector3型の値を保持していれば
		else if (std::holds_alternative<Vector3>(item.value)) {
			// float型のjson配列登録
			Vector3 value = std::get<Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		} // bool型の値を登録
		else if (std::holds_alternative<bool>(item.value)) {
			// bool型の値を登録
			root[groupName][itemName] = std::get<bool>(item.value);
		}
	}

	// ディレクトリが無ければ作成する
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directory(kDirectoryPath);
	}
	// 書き込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 書き込み用ファイルストリーム
	std::ofstream ofs;
	// ファイルを書き込み用に開く
	ofs.open(filePath);

	// ファイルオープン失敗？
	if (ofs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariavles", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む(インデント幅4)
	ofs << std::setw(4) << root << std::endl;
	// ファイルを閉じる
	ofs.close();
}

void GlobalVariables::LoadFiles() {
	// ローカル変数の保存先ファイルパス
	const std::string DirectoryPath = kDirectoryPath;

	// ディレクトリがなければスキップする
	if (!std::filesystem::exists(DirectoryPath)) {
		return;
	}

	std::filesystem::directory_iterator dir_it(DirectoryPath);
	for (const std::filesystem::directory_entry& entry : dir_it) {
		// ファイルパスを取得
		const std::filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		std::string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0) {
			continue;
		}
		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	// 読み込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込み用に開く
	ifs.open(filePath);
	// ファイルオープン失敗？
	if (ifs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariavles", 0);
		assert(0);
		return;
	}
	json root;

	// json文字列からjsonのデータ構造に展開
	ifs >> root;
	// ファイルを閉じる
	ifs.close();

	// グループを検索
	json::iterator itGroup = root.find(groupName);

	// 未登録チェック
	assert(itGroup != root.end());

	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		// アイテム名を取得
		const std::string& itemName = itItem.key();

		// int32_t型の値を保持していれば
		if (itItem->is_number_integer()) {
			// int型の値を登録
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		} // float型の値を保持していれば
		else if (itItem->is_number_float()) {
			// int型の値を登録
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		}// Vector2型の値を保持していれば
		else if (itItem->is_array() && itItem->size() == 2) {
			// float型のjson配列登録
			Vector2 value = { itItem->at(0), itItem->at(1) };
			SetValue(groupName, itemName, value);
		}// Vector3型の値を保持していれば
		else if (itItem->is_array() && itItem->size() == 3) {
			// float型のjson配列登録
			Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		} // bool型の値を保持していれば
		else if (itItem->is_boolean()) {
			// bool型の値を登録
			bool value = itItem->get<bool>();
			SetValue(groupName, itemName, value);
		}
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector2& value)
{
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector3& value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const bool& value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, int32_t value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, float value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector2& value)
{
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector3& value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const bool& value) {
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const {
	// 指定グループが存在するか確認
	if (datas_.find(groupName) == datas_.end()) {
		// グループが存在しない場合は、デフォルト値を返す
		return 0;
	}
	// グループの参照を取得
	const Group& group = datas_.at(groupName);
	// 指定グループに指定のキーが存在するか確認
	assert(group.items.find(key) != group.items.end());
	// アイテムの参照を取得
	const Item& item = group.items.at(key);
	// int32_t型の値を保持しているか確認
	assert(std::holds_alternative<int32_t>(item.value));
	// 指定グループから指定のキーの値を取得
	return std::get<int32_t>(item.value);
}

float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const {
	// 指定グループが存在するか確認
	if (datas_.find(groupName) == datas_.end()) {
		// グループが存在しない場合は、デフォルト値を返す
		return 0;
	}
	// グループの参照を取得
	const Group& group = datas_.at(groupName);
	// 指定グループに指定のキーが存在するか確認
	assert(group.items.find(key) != group.items.end());
	// アイテムの参照を取得
	const Item& item = group.items.at(key);
	// float型の値を保持しているか確認
	assert(std::holds_alternative<float>(item.value));
	// 指定グループから指定のキーの値を取得
	return std::get<float>(item.value);
}

Vector2 GlobalVariables::GetVector2Value(const std::string& groupName, const std::string& key) const
{
	// 指定グループが存在するか確認
	if (datas_.find(groupName) == datas_.end()) {
		// グループが存在しない場合は、デフォルト値を返す
		return Vector2(0);
	}
	// グループの参照を取得
	const Group& group = datas_.at(groupName);
	// 指定グループに指定のキーが存在するか確認
	assert(group.items.find(key) != group.items.end());
	// アイテムの参照を取得
	const Item& item = group.items.at(key);
	// Vector2型の値を保持しているか確認
	assert(std::holds_alternative<Vector2>(item.value));
	// 指定グループから指定のキーの値を取得
	return std::get<Vector2>(item.value);
}

Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const {
	// 指定グループが存在するか確認
	if (datas_.find(groupName) == datas_.end()) {
		// グループが存在しない場合は、デフォルト値を返す
		return Vector3(0);
	}
	// グループの参照を取得
	const Group& group = datas_.at(groupName);
	// 指定グループに指定のキーが存在するか確認
	assert(group.items.find(key) != group.items.end());
	// アイテムの参照を取得
	const Item& item = group.items.at(key);
	// Vector3型の値を保持しているか確認
	assert(std::holds_alternative<Vector3>(item.value));
	// 指定グループから指定のキーの値を取得
	return std::get<Vector3>(item.value);
}

bool GlobalVariables::GetBoolValue(const std::string& groupName, const std::string& key) const {
	// 指定グループが存在するか確認
	if (datas_.find(groupName) == datas_.end()) {
		// グループが存在しない場合は、デフォルト値を返す
		return 0;
	}
	// グループの参照を取得
	const Group& group = datas_.at(groupName);
	// 指定グループに指定のキーが存在するか確認
	assert(group.items.find(key) != group.items.end());
	// アイテムの参照を取得
	const Item& item = group.items.at(key);
	// bool型の値を保持しているか確認
	assert(std::holds_alternative<bool>(item.value));
	// 指定グループから指定のキーの値を取得
	return std::get<bool>(item.value);
}

// 指定されたグループが存在するかを確認するメソッド
bool GlobalVariables::GroupExists(const std::string& groupName) const {
	return datas_.find(groupName) != datas_.end();
}