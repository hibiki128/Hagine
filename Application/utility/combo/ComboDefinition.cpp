#include "ComboDefinition.h"
#include <asset/AssetPath.h>
#include <debug/log/Logger.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Hagine;

namespace {
using json = nlohmann::json;

// JSONのキー名（定義ファイルとの対応をここに集約する）
constexpr const char *kKeyFinishRecovery = "finishRecovery";
constexpr const char *kKeySteps = "steps";
constexpr const char *kKeyAttackName = "attackName";
constexpr const char *kKeyAnimationPath = "animationPath";
constexpr const char *kKeyDamage = "damage";
constexpr const char *kKeyKnockbackPower = "knockbackPower";
constexpr const char *kKeyColliderActiveDuration = "colliderActiveDuration";
constexpr const char *kKeyColliderActivateDelay = "colliderActivateDelay";

constexpr int kJsonIndent = 4; // 書き出し時のインデント幅
} // namespace

std::string ComboDefinition::BuildFilePath(const std::string &fileName)
{
    return AssetPath::JsonRoot() + "/" + kFolderName + "/" + fileName + ".json";
}

bool ComboDefinition::Load(const std::string &fileName)
{
    fileName_ = fileName;
    steps_.clear();
    finishRecovery_ = kDefaultFinishRecovery;

    const std::string filePath = BuildFilePath(fileName);
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
    {
        Logger::Error("Combo definition not found: \"" + filePath + "\"");
        return false;
    }

    json root;
    try
    {
        inFile >> root;
    }
    catch (const json::exception &e)
    {
        Logger::Error("Failed to parse combo definition: \"" + filePath + "\". " + e.what());
        return false;
    }

    finishRecovery_ = root.value(kKeyFinishRecovery, kDefaultFinishRecovery);

    if (!root.contains(kKeySteps) || !root[kKeySteps].is_array())
    {
        Logger::Error("Combo definition has no \"steps\" array: \"" + filePath + "\"");
        return false;
    }

    // 段は配列の並び順がそのままコンボ順になる
    for (const json &stepJson : root[kKeySteps])
    {
        ComboStepData step;
        step.attackName = stepJson.value(kKeyAttackName, std::string());
        step.animationPath = stepJson.value(kKeyAnimationPath, std::string());
        step.damage = stepJson.value(kKeyDamage, step.damage);
        step.knockbackPower = stepJson.value(kKeyKnockbackPower, step.knockbackPower);
        step.colliderActiveDuration = stepJson.value(kKeyColliderActiveDuration, step.colliderActiveDuration);
        step.colliderActivateDelay = stepJson.value(kKeyColliderActivateDelay, step.colliderActivateDelay);
        steps_.push_back(step);
    }

    if (steps_.empty())
    {
        Logger::Error("Combo definition is empty: \"" + filePath + "\"");
        return false;
    }

    return true;
}

bool ComboDefinition::Save() const
{
    if (fileName_.empty())
    {
        return false;
    }

    json root;
    root[kKeyFinishRecovery] = finishRecovery_;
    root[kKeySteps] = json::array();

    for (const ComboStepData &step : steps_)
    {
        json stepJson;
        stepJson[kKeyAttackName] = step.attackName;
        stepJson[kKeyAnimationPath] = step.animationPath;
        stepJson[kKeyDamage] = step.damage;
        stepJson[kKeyKnockbackPower] = step.knockbackPower;
        stepJson[kKeyColliderActiveDuration] = step.colliderActiveDuration;
        stepJson[kKeyColliderActivateDelay] = step.colliderActivateDelay;
        root[kKeySteps].push_back(stepJson);
    }

    const std::string filePath = BuildFilePath(fileName_);
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

    std::ofstream outFile(filePath);
    if (!outFile.is_open())
    {
        Logger::Error("Failed to write combo definition: \"" + filePath + "\"");
        return false;
    }

    outFile << root.dump(kJsonIndent);
    return true;
}
