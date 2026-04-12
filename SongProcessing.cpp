// SongProcessing.cpp
// 功能：读取歌曲列表文件，去重排序后覆写并生成统计信息，英文歌曲名首字母大写、其余字母小写处理
// 平台：Windows
// 标准：C++17

#include <windows.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

// ------------------------- 常量定义 -------------------------
constexpr std::string_view INPUT_FILE   = "music.txt";
constexpr std::string_view TEMP_FILE    = "music.temp";
constexpr std::string_view HEADER_LINE  = "歌名";
constexpr std::string_view STATS_PREFIX = "共";

// ------------------------- 辅助函数 -------------------------
[[nodiscard]] inline bool isStatisticsLine(std::string_view line) noexcept {
    // 统计行特征：以"共"开头且紧随数字
    if (line.size() < 4) return false;
    return (line.substr(0, 3) == STATS_PREFIX) &&
           (std::isdigit(static_cast<unsigned char>(line[3])));
}

// 将英文歌曲名规范为：每个单词首字母大写，其余字母小写
// 非 ASCII 字符（如中文）保持原样
[[nodiscard]] std::string normalizeEnglishTitle(std::string_view title) {
    std::string result;
    result.reserve(title.size());
    bool newWord = true;
    for (char c : title) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc)) {
            // 仅处理 ASCII 字母，非 ASCII 字符不改变大小写
            if (uc < 0x80) {
                result.push_back(newWord ? std::toupper(uc) : std::tolower(uc));
            } else {
                result.push_back(c);
            }
            newWord = false;
        } else {
            result.push_back(c);
            // 单词分隔符：空白字符、连字符、撇号视为新单词开始
            if (std::isspace(uc) || uc == '-' || uc == '\'') {
                newWord = true;
            }
        }
    }
    return result;
}

// ------------------------- 主处理流程 -------------------------
int main() {
    // 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // 打开原始歌曲列表文件
    std::ifstream inFile(INPUT_FILE.data());
    if (!inFile) {
        std::cerr << "错误: 无法打开 " << INPUT_FILE << "，请确保文件存在且未被占用\n";
        return 1;
    }

    // 创建临时文件（覆盖写入）
    std::ofstream outFile(TEMP_FILE.data(), std::ios::trunc);
    if (!outFile) {
        std::cerr << "错误: 无法创建临时文件 " << TEMP_FILE << "，请检查磁盘权限或空间\n";
        return 1;
    }

    std::string line;
    // 跳过第一行表头
    if (!std::getline(inFile, line)) {
        std::cerr << "警告: 文件内容为空\n";
    }
    // 跳过第二行（通常为空行或分隔符）
    if (!std::getline(inFile, line)) {
        std::cerr << "警告: 文件内容不足两行，可能缺少必要的表头信息\n";
    }

    // 读取所有歌曲条目，去重排序，并对英文名规范化
    std::set<std::string> songs;
    while (std::getline(inFile, line)) {
        if (isStatisticsLine(line)) break;   // 遇到统计行则停止读取
        if (!line.empty()) {
            // 插入前进行英文大小写规范化
            songs.insert(normalizeEnglishTitle(line));
        }
    }
    inFile.close();

    if (songs.empty()) {
        std::cerr << "警告: 未读取到任何歌曲条目，输出文件将只包含统计信息\n";
    }

    // 写入新文件内容
    outFile << HEADER_LINE << "\n\n";
    for (const auto& song : songs) {
        outFile << song << '\n';
    }
    // 末尾空行与统计信息
    outFile << "\n\n\n\n\n"
            << STATS_PREFIX << songs.size()
            << "首，重新运行本程序以刷新统计数据。";

    outFile.close();

    // 用临时文件替换原文件（原子性重命名）
    std::error_code ec;
    fs::rename(TEMP_FILE, INPUT_FILE, ec);
    if (ec) {
        std::cerr << "错误: 无法将临时文件重命名为正式文件 (" << ec.message() << ")\n"
                  << "临时文件保留在: " << TEMP_FILE << "\n"
                  << "您可以手动将其重命名为 " << INPUT_FILE << " 或检查文件占用情况\n";
        return 1;
    }

    std::cout << "歌曲列表处理完成，共 " << songs.size() << " 首歌曲\n";
    return 0;
}
