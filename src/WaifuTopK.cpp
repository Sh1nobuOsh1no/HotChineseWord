#include "WaifuTopK.h"

#include <fstream>
#include <sstream>

// ===== ctor =====
HotWordSystem::HotWordSystem(const std::string& dict_dir, int window_duration_sec, int bucket_step_sec)
    : window_duration_sec_(window_duration_sec),
      bucket_step_sec_(bucket_step_sec) {

    const std::string dict_path  = dict_dir + "/jieba.dict.utf8";
    const std::string hmm_path   = dict_dir + "/hmm_model.utf8";
    const std::string user_path  = dict_dir + "/user.dict.utf8";
    const std::string idf_path   = dict_dir + "/idf.utf8";
    const std::string stop_path  = dict_dir + "/stop_words.utf8";

    jieba_ = std::make_unique<cppjieba::Jieba>(
        dict_path, hmm_path, user_path, idf_path, stop_path
    );

    // 从 user.dict 读取 waifu 完整名，并生成别名映射
    load_waifu_from_user_dict(user_path);
    build_waifu_alias_map();

    // 完整名按长度降序：用于“完整优先”
    waifu_full_names_sorted_desc_ = waifu_full_names_;
    std::sort(waifu_full_names_sorted_desc_.begin(),
              waifu_full_names_sorted_desc_.end(),
              [](const std::string& a, const std::string& b) {
                  return a.size() > b.size();
              });
}

// ===== Interning =====
uint64_t HotWordSystem::get_or_create_word_id(const std::string& word) {
    auto it = word_to_id_.find(word);
    if (it != word_to_id_.end()) return it->second;

    uint64_t new_id = next_word_id_++;
    word_to_id_[word] = new_id;
    id_to_word_[new_id] = word;
    return new_id;
}

std::string HotWordSystem::get_word_by_id(uint64_t id) const {
    auto it = id_to_word_.find(id);
    if (it != id_to_word_.end()) return it->second;
    return "";
}

// ===== waifu dict load =====
void HotWordSystem::load_waifu_from_user_dict(const std::string& user_dict_path) {
    std::ifstream fin(user_dict_path);
    if (!fin.is_open()) {
        std::cerr << "[HotWordSystem] Warning: cannot open user dict: "
                  << user_dict_path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        // user.dict 常见格式：word [freq] [tag]
        std::istringstream iss(line);
        std::string word;
        iss >> word;

        if (word.size() < 2) continue; // 至少两个字符

        // 去重
        if (waifu_full_set_.insert(word).second) {
            waifu_full_names_.push_back(word);
        }
    }
}

// 生成别名(>=2) -> 完整名 映射
// 规则：若别名对应多个完整名，剔除该别名（避免误判）
void HotWordSystem::build_waifu_alias_map() {
    std::unordered_map<std::string, std::string> tmp;      // alias -> full
    std::unordered_set<std::string> conflicted_alias;      // 冲突的 alias

    for (const auto& full : waifu_full_names_) {
        const int n = static_cast<int>(full.size());
        for (int i = 0; i < n; ++i) {
            for (int len = 2; len <= n - i; ++len) {
                std::string alias = full.substr(i, len);

                // 完整名本身也作为 alias（这样缩写统一走同一套逻辑）
                // 注意：完整优先是在匹配阶段做的（防止重复计数）
                auto it = tmp.find(alias);
                if (it == tmp.end()) {
                    tmp.emplace(alias, full);
                } else {
                    if (it->second != full) {
                        conflicted_alias.insert(alias);
                    }
                }
            }
        }
    }

    alias_to_full_.clear();
    alias_to_full_.reserve(tmp.size());

    for (const auto& kv : tmp) {
        if (conflicted_alias.find(kv.first) != conflicted_alias.end()) continue;
        alias_to_full_.emplace(kv.first, kv.second);
    }
}

// 在消息中统计 waifu：
// 1) 先做“完整名优先”匹配（按长度降序），匹配到就计数并“遮罩”该段
// 2) 对剩余文本做 jieba 分词；token >=2 且在 alias_to_full_ 中，则计到 canonical(完整名)
void HotWordSystem::count_waifu_in_message(const std::string& content, TimeBucket& bucket) {
    if (content.empty()) return;

    // 遮罩：用一个同长度的标记串，标记哪些位置已经被“完整名”占用
    std::string used(content.size(), '\0');

    auto mark_used = [&](size_t pos, size_t len) {
        for (size_t i = pos; i < pos + len && i < used.size(); ++i) used[i] = 1;
    };
    auto is_unused_range = [&](size_t pos, size_t len) -> bool {
        if (pos + len > used.size()) return false;
        for (size_t i = pos; i < pos + len; ++i) if (used[i]) return false;
        return true;
    };

    // 1) 完整名优先计数（不会让其被后续 alias 再次命中）
    for (const auto& full : waifu_full_names_sorted_desc_) {
        if (full.size() < 2) continue;

        size_t start = 0;
        while (true) {
            size_t pos = content.find(full, start);
            if (pos == std::string::npos) break;

            if (is_unused_range(pos, full.size())) {
                uint64_t wid = get_or_create_word_id(full);
                bucket.word_counts[wid]++;
                global_count_[wid]++;
                mark_used(pos, full.size());
            }

            start = pos + 1; // 允许重叠搜索（但 used 会防止重复计数同一段）
        }
    }

    // 2) 对未被完整名覆盖的部分，使用分词 token 做别名命中
    std::vector<std::string> words;
    jieba_->Cut(content, words, false);
    if (words.empty()) return;

    for (const auto& token : words) {
        if (token.size() < 2) continue;

        // token 必须是 waifu 的 alias（包含完整名本身）
        auto it = alias_to_full_.find(token);
        if (it == alias_to_full_.end()) continue;

        const std::string& canonical_full = it->second;

        // 如果 token 本身就是完整名，那前面“完整优先”已经计过了；
        // 为避免重复，这里跳过完整名 token（完整计数以步骤1为准）
        if (waifu_full_set_.find(token) != waifu_full_set_.end()) {
            continue;
        }

        // 缩写/子串命中：计到 canonical(完整名)
        uint64_t wid = get_or_create_word_id(canonical_full);
        bucket.word_counts[wid]++;
        global_count_[wid]++;
    }
}

// ===== main pipeline =====
void HotWordSystem::add_message(const std::string& content, long long timestamp_ms) {
    if (content.empty()) return;

    long long timestamp_sec = timestamp_ms / 1000;
    long long align_time = timestamp_sec - (timestamp_sec % bucket_step_sec_);

    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (window_buckets_.empty() || window_buckets_.back().align_timestamp != align_time) {
        if (!window_buckets_.empty() && align_time < window_buckets_.back().align_timestamp) {
            // 乱序处理：这里保持你的原注释（可按需扩展）
            // 简化：仍然将其放入当前最后一个桶（不建议但保持兼容）
            // 若你需要严格乱序插入，可改为二分找到对应桶。
            // 这里直接落到 back()
        } else {
            window_buckets_.push_back({align_time, {}});
        }
    }

    TimeBucket& current_bucket = window_buckets_.back();

    // 只统计 waifu
    count_waifu_in_message(content, current_bucket);

    // window_length = -1 时，不滑窗不过期
    if (window_duration_sec_ != -1) {
        slide_window(align_time);
    }
}

void HotWordSystem::slide_window(long long current_align_time) {
    long long expire_threshold = current_align_time - window_duration_sec_;

    while (!window_buckets_.empty()) {
        TimeBucket& front = window_buckets_.front();

        if (front.align_timestamp <= expire_threshold) {
            for (const auto& kv : front.word_counts) {
                auto it = global_count_.find(kv.first);
                if (it != global_count_.end()) {
                    it->second -= kv.second;
                    if (it->second <= 0) {
                        global_count_.erase(it);
                        // 不清理 id_to_word_：避免频繁维护字典表
                    }
                }
            }
            window_buckets_.pop_front();
        } else {
            break;
        }
    }
}

std::vector<WordFreq> HotWordSystem::query_top_k(int k) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (global_count_.empty() || k <= 0) return {};

    using Node = std::pair<int, uint64_t>; // <count, id>
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> min_heap;

    for (const auto& kv : global_count_) {
        int count = kv.second;
        uint64_t id = kv.first;
        if (count <= 0) continue;

        if (min_heap.size() < static_cast<size_t>(k)) {
            min_heap.push({count, id});
        } else if (count > min_heap.top().first) {
            min_heap.pop();
            min_heap.push({count, id});
        }
    }

    std::vector<WordFreq> result;
    result.reserve(min_heap.size());

    while (!min_heap.empty()) {
        auto node = min_heap.top();
        min_heap.pop();
        result.push_back({get_word_by_id(node.second), node.first});
    }

    std::reverse(result.begin(), result.end());
    return result;
}
