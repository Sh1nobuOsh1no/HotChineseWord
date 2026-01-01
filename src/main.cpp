#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include "WaifuTopK.h"

std::pair<long long, std::string> parse_chat_line(const std::string& line) {
    static const std::regex pattern(R"(\s*\[(\d+):(\d+):(\d+)\]\s*(.*))");

    std::smatch matches;
    if (std::regex_search(line, matches, pattern)) {
        long long h = std::stoll(matches[1].str());
        long long m = std::stoll(matches[2].str());
        long long s = std::stoll(matches[3].str());
        std::string msg = matches[4].str();

        if (!msg.empty() && msg.back() == '\r') msg.pop_back();
        return {h * 3600 + m * 60 + s, msg};
    }
    return {0, ""};
}

int main() {
    std::string dict_path = "dict";

    int length;
    std::cout << "请输入窗口长度(秒). -1 表示不过期不滑窗:" << std::endl;
    std::cin >> length;

    if (std::cin.fail()) {
        std::cerr << "输入错误，请确保输入一个数字。" << std::endl;
        return 1;
    }

    HotWordSystem hws(dict_path, length);

    std::string filename;

    std::cout << "请输入" << std::endl;
    std::cout << "1: input1.txt" << std::endl;
    std::cout << "2: input2.txt" << std::endl;
    std::cout << "3: input3.txt" << std::endl;
    std::cout << "请选择文件编号: ";

    int x;
    std::cin >> x;
    if (x == 1) filename = "input1.txt";
    else if (x == 2) filename = "input2.txt";
    else if (x == 3) filename = "input3.txt";
    else {
        std::cerr << "无效选择，将使用默认文件 input1.txt" << std::endl;
        filename = "input1.txt";
    }

    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(infile, line)) {
        auto [timestamp, content] = parse_chat_line(line);
        if (!content.empty()) {
            // timestamp 这里是“秒”，add_message 期望“毫秒”
            // 你原代码是直接传 timestamp（秒）给 timestamp_ms，这里修正为 *1000
            hws.add_message(content, timestamp * 1000);
        }
    }
    infile.close();

    std::cout << "热词查询到第几位:" << std::endl;
    int k;
    std::cin >> k;

    std::vector<WordFreq> top_words = hws.query_top_k(k);

    std::cout << "Top " << k << " Waifu Names:" << std::endl;
    for (const auto& wf : top_words) {
        std::cout << wf.word << ": " << wf.count << std::endl;
    }

    return 0;
}
