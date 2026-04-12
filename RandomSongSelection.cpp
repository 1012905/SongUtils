// RandomSongSelection.cpp
// 功能：随机选择一首歌曲并打开 B 站搜索对应关键词
// 平台：Windows
// 标准：C++17

#include <windows.h>
#include <shellapi.h>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "shell32.lib")

// ------------------------- 常量定义 -------------------------
constexpr std::string_view INPUT_FILE      = "music.txt";          // 歌曲列表文件名
constexpr std::string_view SEARCH_URL_TMPL = "https://search.bilibili.com/all?keyword=";
constexpr std::string_view HEADER_LINE     = "歌名";               // 表头标识
constexpr std::string_view STATS_PREFIX    = "共";                // 统计行前缀

// ------------------------- 辅助函数 -------------------------
/// 判断是否为统计行（以“共”+数字开头）
[[nodiscard]] inline bool isStatisticsLine(std::string_view line) noexcept {
    if (line.size() < 4) return false;
    return (line.substr(0, 3) == STATS_PREFIX) &&
           (std::isdigit(static_cast<unsigned char>(line[3])));
}

/// URL 编码（UTF-8 字符串）
[[nodiscard]] std::string urlEncode(std::string_view str) {
    std::ostringstream escaped;
    escaped << std::hex << std::uppercase;

    for (unsigned char c : str) {
        if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << std::setfill('0')
                    << static_cast<int>(c);
        }
    }
    return escaped.str();
}

/// 使用默认浏览器打开指定 URL
void openUrlInBrowser(std::string_view utf8Url) {
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Url.data(),
                                      static_cast<int>(utf8Url.size()), nullptr, 0);
    if (wideLen <= 0) return;

    std::wstring wideUrl(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Url.data(),
                        static_cast<int>(utf8Url.size()), wideUrl.data(), wideLen);

    ShellExecuteW(nullptr, L"open", wideUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// ------------------------- 歌曲列表加载 -------------------------
/// 从文件中读取歌曲名称，跳过表头与统计行
[[nodiscard]] std::vector<std::string> loadSongsFromFile(std::string_view filename) {
    std::vector<std::string> songs;
    std::ifstream inFile(filename.data());
    if (!inFile) {
        std::cerr << "错误: 无法打开 " << filename << "，请确保文件存在\n";
        return songs;
    }

    std::string line;
    // 跳过前两行（表头及其后的空行）
    if (!std::getline(inFile, line) || !std::getline(inFile, line)) {
        std::cerr << "警告: " << filename << " 格式异常，可能缺少表头\n";
    }

    while (std::getline(inFile, line)) {
        if (isStatisticsLine(line)) break;   // 遇到统计行则停止读取
        if (!line.empty()) songs.push_back(line);
    }
    return songs;
}

// ------------------------- 随机选择 -------------------------
/// 从歌曲列表中随机选取一首
[[nodiscard]] std::string pickRandomSong(const std::vector<std::string>& songs) {
    // 使用 thread_local 随机生成器避免重复初始化随机设备
    static thread_local std::mt19937 gen{ std::random_device{}() };
    std::uniform_int_distribution<size_t> dist(0, songs.size() - 1);
    return songs[dist(gen)];
}

// ------------------------- 程序入口 -------------------------
int main() {
    SetConsoleOutputCP(CP_UTF8);   // 控制台输出 UTF-8

    auto songs = loadSongsFromFile(INPUT_FILE);
    if (songs.empty()) {
        std::cerr << "错误: 未读取到任何歌曲，无法随机选择\n";
        return 1;
    }

    std::string selected = pickRandomSong(songs);
    std::string url = std::string(SEARCH_URL_TMPL) + urlEncode(selected);
    openUrlInBrowser(url);

    std::cout << "本次随机歌曲: " << selected << '\n';
    return 0;
}