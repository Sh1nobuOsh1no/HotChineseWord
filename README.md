# scJieBa：用于 Waifu 的 Top-K 获取

![Version](https://img.shields.io/badge/version-0.5-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Build](https://img.shields.io/badge/build-xmake-green)

> 一个基于 C++ + cppjieba 的练习项目：从弹幕/聊天记录中提取并统计 waifu 名字，支持 Top-K 查询和滑动窗口统计。

## 项目概览

本项目通过中文分词技术，从聊天记录中识别并统计动漫角色（Waifu）名称的出现频率，支持热度排行查询和时间窗口统计。

**在 Windows 上运行时请注意：**
```
chcp 65001  # 切换控制台到 UTF-8 编码
```

## 功能特性

- 读取聊天记录（格式：`[hh:mm:ss] message`）
- 使用自定义词典 `dict/user.dict.utf8` 作为 waifu 名字库
- 智能统计 waifu 名字（不统计其他分词结果）
- 支持 Top-K 查询（最热 waifu 排行榜）
- 灵活的窗口统计：
    - `window_length = -1`：全量累计统计（不过期）
    - `window_length > 0`：按时间窗口滑动过期

## 项目结构

```
.
├── dict/
│   ├── jieba.dict.utf8
│   ├── hmm_model.utf8
│   ├── user.dict.utf8        # 你的 waifu 字典（关键）
│   ├── idf.utf8
│   └── stop_words.utf8
├── input1.txt
├── input2.txt
├── input3.txt
├── HotWordSystem.h
├── HotWordSystem.cpp
└── main.cpp
```

## Waifu 字典配置

`dict/user.dict.utf8` 是 waifu 名字表，支持以下格式：

```
雪之下雪乃
雪之下雪乃 10000
雪之下雪乃 10000 nz
# 以 # 开头的行会被忽略
```

**约束条件：**
- 名字长度必须 ≥ 2
- 词典中的名字被视为"完整名（canonical）"

##  统计规则（V0.5）

V0.5 版本采用"waifu 专用计数"逻辑：

### 1. 只记录 waifu 名字
- 非 waifu 词一律不进入统计

### 2. 完整名优先
- 若消息中出现完整名字，优先按完整名计数
- 对该文本片段做"遮罩"，避免被别名重复计数

### 3. 允许不完整命中（≥2 字）
- 由每个完整名生成所有长度 ≥2 的子串作为"别名"
- 例如完整名"雪之下雪乃"的别名包括：雪之、雪乃、之下、下雪 等
- 命中别名时，统一归并计到对应的完整名

### 4. 别名去歧义
- 如果某个别名同时对应多个完整名（冲突），该别名会被剔除
- 采用"宁可少算也不乱算"的策略

### 5. 窗口统计
- `window_length = -1` 表示永久累计：不调用 slide，不做过期淘汰
- `window_length > 0` 表示按时间窗口滑动统计

## 使用方法

程序运行后依次输入：

1. 窗口长度（秒）
    - 输入 `-1`：全量累计
    - 输入正数：滑动窗口统计
2. 选择输入文件编号（1/2/3）
3. 输入 K（Top-K）

**输出示例：**
```
Top K Waifu Names:
雪之下雪乃: 12
...
```

## 构建与运行

本项目使用 xmake 构建系统：

```bash
xmake       # 构建项目
xmake run   # 运行程序
```

## 版本记录

### V0.5
- 引入 waifu 专用统计
- 使用 `dict/user.dict.utf8` 作为 waifu 完整名词典
- 只统计 waifu 名字
- 完整名优先 + 别名归并（≥2 字）
- 别名冲突自动剔除
- `window_length = -1` 时不滑窗不过期

### V0.4
- 整体优化：通过 `word_to_id_` 将字符串映射为 uint64_t ID   
- 显著提升 unordered_map 性能并降低内存占用

### V0.3
- 优化 `query_top_k`：采用 Min-Heap 替代全量排序，提高查询效率

### V0.2
- 增加输入选择：从三个 input 文件中选择其一

### V0.1
- 实现基本分词功能

## 注意事项

- Windows 控制台务必使用 UTF-8（`chcp 65001`），否则中文显示可能乱码
- 如果 waifu 名字之间高度相似，别名冲突会增多，导致某些简称不计数
- 这是"宁可少算也不乱算"的策略，确保统计准确性

## 示例代码片段

```cpp
// HotWordSystem.h 核心接口示例
class HotWordSystem {
public:
    HotWordSystem(const std::string& dict_path, int window_length = -1);
    void process_message(const std::string& timestamp, const std::string& message);
    void slide(const std::string& current_timestamp);
    std::vector<std::pair<std::string, int>> query_top_k(int k);
};
```

## 可能的扩展方向

- 支持多线程处理大规模聊天记录
- 添加可视化界面展示 waifu 热度变化
- 实现更复杂的时间窗口（如按小时/天/周统计）
- 增加情感分析，区分正面/负面提及