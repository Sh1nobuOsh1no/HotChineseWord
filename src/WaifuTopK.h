#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <queue>
#include <string>

#include "cppjieba/Jieba.hpp"

// 返回值结构保持不变
struct WordFreq {
    std::string word;
    int count;
};

class HotWordSystem {
public:
    HotWordSystem(const std::string& dict_dir, int window_duration_sec, int bucket_step_sec = 1);

    // 禁止拷贝
    HotWordSystem(const HotWordSystem&) = delete;
    HotWordSystem& operator=(const HotWordSystem&) = delete;

    void add_message(const std::string& content, long long timestamp_ms);
    std::vector<WordFreq> query_top_k(int k);

private:
    // [更改] 内部存储不再存 string，而是存 uint64_t (Word ID)
    struct TimeBucket {
        long long align_timestamp;
        std::unordered_map<uint64_t, int> word_counts;
    };

    // Interning: string <-> id
    uint64_t get_or_create_word_id(const std::string& word);
    std::string get_word_by_id(uint64_t id) const;

    void slide_window(long long current_align_time);

    // ===== waifu 名字逻辑 =====
    void load_waifu_from_user_dict(const std::string& user_dict_path);
    void build_waifu_alias_map(); // 由完整名生成 >=2 的子串别名(去歧义)
    void count_waifu_in_message(const std::string& content, TimeBucket& bucket);

private:
    int window_duration_sec_;
    int bucket_step_sec_;

    std::deque<TimeBucket> window_buckets_;

    std::unordered_map<uint64_t, int> global_count_;

    std::unordered_map<std::string, uint64_t> word_to_id_;
    std::unordered_map<uint64_t, std::string> id_to_word_;
    uint64_t next_word_id_ = 1;

    std::unique_ptr<cppjieba::Jieba> jieba_;
    mutable std::shared_mutex mutex_;

    // user.dict 里读到的“完整 waifu 名字”
    std::vector<std::string> waifu_full_names_;

    // 完整名集合：用于“完整优先”
    std::unordered_set<std::string> waifu_full_set_;

    // 别名(>=2) -> canonical(完整名)
    // 若别名冲突（对应多个完整名），会被剔除，不参与统计（避免误判）
    std::unordered_map<std::string, std::string> alias_to_full_;

    // 为了“完整优先”，匹配时按长度降序扫描
    std::vector<std::string> waifu_full_names_sorted_desc_;
};
