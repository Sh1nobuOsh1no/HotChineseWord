#include <iostream>
#include <string>
#include <vector>
#include <fstream> 
#include <regex>  
#include "HotWordSystem.h" 

std::pair<long long, std::string> parse_chat_line(const std::string& line) {
    std::regex pattern("\\[(\\d+):(\\d+):(\\d+)\\]\\s*(.*)");
    std::smatch matches;
    if (std::regex_match(line, matches, pattern)) {
        long long hours = std::stoll(matches[1].str());
        long long minutes = std::stoll(matches[2].str());
        long long seconds = std::stoll(matches[3].str());
        std::string message = matches[4].str();
        long long timestamp_in_seconds = hours * 3600 + minutes * 60 + seconds;
        return {timestamp_in_seconds, message};
    }
    return {0, ""}; 
}

int main() {
    std::string dict_path = "dict";
    int length;
    std::cout<<"请输入窗口长度:"<<std::endl;
    std::cin>>length;
    HotWordSystem hws(dict_path,length);
    std::string filename; 

    std::cout << "请输入" << std::endl; 
    std::cout << "1: input1.txt" << std::endl;
    std::cout << "2: input2.txt" << std::endl;
    std::cout << "3: input3.txt" << std::endl;
    std::cout << "请选择文件编号: "; 
    
    int x;
    std::cin >> x;
    if (x == 1) {
        filename = "input1.txt"; 
    } else if (x == 2) {
        filename = "input2.txt";
    } else if (x == 3) {
        filename = "input3.txt"; 
    } else {
        std::cerr << "无效选择，将使用默认文件 input1.txt" << std::endl;
        filename = "input1.txt"; 
    }
    
    if (std::cin.fail()) {
        std::cerr << "输入错误，请确保输入一个数字。" << std::endl;
        return 1;
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
            hws.add_message(content,timestamp);
        }
    }

    infile.close();

    std::cout<<"热词查询到第几位:"<<std::endl;
    int k;
    std::cin>>k;
    std::vector<WordFreq> top_words = hws.query_top_k(k);

    std::cout << "Top " << k << " Hot Words:" << std::endl;
    for (const auto& wf : top_words) {
        std::cout << wf.word << ": " << wf.count << std::endl;
    }

    return 0;
}
