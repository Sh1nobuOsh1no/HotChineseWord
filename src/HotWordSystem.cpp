#include "HotWordSystem.h"

using namespace std;

HotWordSystem::HotWordSystem(string& dict_dir,long long window_length):window_length(window_length){
    string dict = dict_dir + "/jieba.dict.utf8";
    string hmm = dict_dir +"/hmm_model.utf8";
    string user = dict_dir +"/user.dict.utf8";
    string idf = dict_dir +"/idf.utf8";
    string stop = dict_dir +"/stop_words.utf8";
    jieba_ = std::make_unique<cppjieba::Jieba>(dict, hmm, user, idf, stop);
}

void HotWordSystem::add_message(string& content,long long timestamp){
    vector<string> words;
    jieba_->Cut(content,words,false);

    //检查时间戳是否相等
    if(window_buckets.empty()||window_buckets.back().timestamp!=timestamp){
        window_buckets.push_back({timestamp,{}});
    }

    TimeBucket& current_bucket=window_buckets.back();
    for (const auto& w : words) {
        if(w.size()<=3)continue;
        current_bucket.word_counts[w]++;
        global_count[w]++;
    }

    slide_window(timestamp);
}

//从队尾开始，将 和当前时间差 大于窗口长度 的 时间戳 出队
void HotWordSystem::slide_window(long long current_time){
    long long expire_threshold = current_time - window_length;
    while(!window_buckets.empty()){
        TimeBucket& front=window_buckets.front();
        if(front.timestamp<=expire_threshold){
            //kv的first是字，second是数量
            for(auto& kv:front.word_counts){
                global_count[kv.first]-=kv.second;
                if(global_count[kv.first] <= 0){
                    global_count.erase(kv.first);
                }
            }
            window_buckets.pop_front();
        }else{
            break;
        }
    }
}

vector<WordFreq> HotWordSystem::query_top_k(int k){
    //此处修改:vector<WordFreq> result(global_count.size());
    vector<WordFreq> result;
    result.reserve(global_count.size());

    //将全局记录的值转移到result当中(以便排序)
    for (const auto& kv:global_count) {
        result.push_back({kv.first, kv.second});
    }

    //分两种情况排序
    if(k<result.size()){
        partial_sort(result.begin(),result.begin()+k,result.end(),[](const WordFreq& a,const WordFreq&b){
            return a.count>b.count;
        });
        result.resize(k);
    }else{
        sort(result.begin(),result.end(),[](const WordFreq& a,const WordFreq& b){
            return a.count>b.count;
        });
    }
    return result;
}