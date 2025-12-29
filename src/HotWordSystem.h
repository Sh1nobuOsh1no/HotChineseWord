#pragma once

#include<iostream>
#include<vector>
#include<algorithm>
#include<unordered_map>
#include<deque>
#include"cppjieba/Jieba.hpp"
#include <memory> 

//定义一个时间桶(基本单位）
struct TimeBucket{
    long long timestamp;
    std::unordered_map<std::string,int> word_counts;
};

//返回值topk的结构
struct WordFreq{
    std::string word;
    int count;
};

//核心类
struct HotWordSystem{
public:
    HotWordSystem(std::string& dict,long long window_length);
    void add_message(std::string& content,long long timestamp);
    void slide_window(long long current_time);
    std::vector<WordFreq> query_top_k(int k);
private:
    long long window_length;
    std::deque<TimeBucket> window_buckets;
    std::unordered_map<std::string,int> global_count;
    std::unique_ptr<cppjieba::Jieba> jieba_;
};
