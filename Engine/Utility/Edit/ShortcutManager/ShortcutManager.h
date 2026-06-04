#pragma once
#include "Windows.h"
#include <functional>
#include <string>
#include <unordered_map>
namespace Hagine {
class Input;
class ShortcutManager {
  private:
    ShortcutManager() = default;
    ~ShortcutManager() = default;
    ShortcutManager(ShortcutManager &) = delete;
    ShortcutManager &operator=(ShortcutManager &) = delete;

  public:
    static ShortcutManager *GetInstance() {
        static ShortcutManager instance;
        return &instance;
    }

    void RegisterShortcut(const std::string &name, BYTE key, std::function<void()> callback);
    void RegisterShortcut(const std::string &name, const std::vector<BYTE> &keys, std::function<void()> callback);

    void Initialize(Input *input);

    // 毎フレーム更新処理（Inputからトリガーを取得してコマンド実行）
    void Update();

    void Finalize();

  private:
    // ショートカット情報の構造体（拡張性を持たせる）
    struct Shortcut {
        std::vector<BYTE> keys;
        std::function<void()> callback;
    };

    std::unordered_map<std::string, Shortcut> shortcuts_;

    Input *input_ = nullptr;
};
} // namespace Hagine
