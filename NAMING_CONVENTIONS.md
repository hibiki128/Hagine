## 基本
### オールマン スタイル
```c++
void Main::Function
{
  // Do something...
}
```
### キャストはC++準拠の書き方で
```c++
unsigned int countFrame = 32u;
float time = static_cast<float>(countFrame) / kTargetFrame;
SetTime(static_cast<double>(time));
```
## 命名規則
### メンバ変数は末尾に_(アンダースコア)をつける
```c++
WorldTransform worldTransform_ = {};
uint32_t hImage_ = 0u;
```
### 仮引数名は_(アンダースコア)を最初に**つけない**
```c++
void Copy(char* destination, char* source);
void SetCount(unsigned int count);
```
### 変数がポインタの場合、なるべく頭にpをつける
```c++
int* pNum = nullptr;
Audio* pAudio = Audio::GetInstance();
std::unique_ptr<Japan> pJapan = nullptr;
```
### enum classのメンバーはアッパーキャメルケースで命名する
頭に型名をつけなくて良い
```c++
enum class Scenes {
  Title, Select, Game, Result, Credit
};
```
## ファイル
### ヘッダーファイルとソースファイルは拡張子を除くファイル名を一致させる
```bash
C:\Project\app\entity\Player.h
C:\Project\app\entity\Player.cpp
```
### ファイル名はアッパーキャメルケースで命名する
```bash
PlayerMove.h
ThisIsSomethingToDoSomething.h
```
### フォルダの命名
- 抽象的にする (カテゴリのようなイメージ)
- すべて小文字
- 二語繋げない (できれば)
```bash
./app/entity/
./app/presentation/
./app/logic/
./app/event/
```
### includeのパスはルート基準で指定する
```c++
/// ルートをsrcフォルダとすると、srcの子ディレクトリから記述を始める
#include <logic/MathFunctions.h>
#include <presentation/animation/NumericAnimation.h>
#include <entity/Player.h>
```
- ただし例外として、とあるクラスのソースファイルがそのクラスのヘッダーファイルをインクルードする際はヘッダーファイルからの相対パスで記述する
```c++
/// このファイルはPlayer.cppである
#include "Player.h"
#include <entity/component/EntityMovement.h>
#include <entity/component/EntityStats.h>
```