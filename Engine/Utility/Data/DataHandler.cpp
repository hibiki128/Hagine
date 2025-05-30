#include "DataHandler.h"

DataHandler::DataHandler(const std::string &folder, const std::string &file) {
    folderPath = basePath + "/" + folder;
    fileName = file + ".json";
    fs::create_directories(folderPath); // フォルダを作成
}

// 明示的なテンプレートインスタンス化
template void DataHandler::Save<int>(const std::string &, const int &);
template void DataHandler::Save<int32_t>(const std::string &, const int32_t &);
template void DataHandler::Save<float>(const std::string &, const float &);
template void DataHandler::Save<std::string>(const std::string &, const std::string &);
template void DataHandler::Save<Vector2>(const std::string &, const Vector2 &);
template void DataHandler::Save<Vector3>(const std::string &, const Vector3 &);
template void DataHandler::Save<Vector4>(const std::string &, const Vector4 &);
template void DataHandler::Save<PrimitiveType>(const std::string &, const PrimitiveType &);

template int DataHandler::Load<int>(const std::string &, const int &);
template int32_t DataHandler::Load<int32_t>(const std::string &, const int32_t &);
template float DataHandler::Load<float>(const std::string &, const float &);
template std::string DataHandler::Load<std::string>(const std::string &, const std::string &);
template Vector2 DataHandler::Load<Vector2>(const std::string &, const Vector2 &);
template Vector3 DataHandler::Load<Vector3>(const std::string &, const Vector3 &);
template Vector4 DataHandler::Load<Vector4>(const std::string &, const Vector4 &);
template PrimitiveType DataHandler::Load<PrimitiveType>(const std::string &, const PrimitiveType &);
