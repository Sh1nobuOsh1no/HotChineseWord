# JieBa的简单尝试，热词Top-k获取
### 本项目使用xmake构建，语言上选择cpp，windows上运行要注意切换为UTF-8
```cmd
chcp 65001
```
---
# V0.1
实现了基本的分词功能

# V0.2
增加了输入选择（从三个input文件当中选一）

# V0.3
优化了query_top_k的算法，采用Min-Heap替代全量排序
