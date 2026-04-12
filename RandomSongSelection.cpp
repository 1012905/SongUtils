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
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "shell32.lib")

// ------------------------- 常量定义 -------------------------
constexpr std::string_view PROCESSOR_EXE    = "SongProcessing.exe";
constexpr std::string_view INPUT_FILE       = "music.txt";
constexpr std::string_view SEARCH_URL_TMPL  = "https://search.bilibili.com/all?keyword=";
constexpr std::string_view HEADER_LINE      = "歌名";
constexpr std::string_view STATS_PREFIX     = "共";

// ------------------------- 辅助函数 -------------------------
[[nodiscard]] inline bool isStatisticsLine(std::string_view line) noexcept {
    if (line.size() < 4) return false;
    return (line.substr(0, 3) == STATS_PREFIX) &&
           (std::isdigit(static_cast<unsigned char>(line[3])));
}

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

void openUrlInBrowser(std::string_view utf8Url) {
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Url.data(),
                                      static_cast<int>(utf8Url.size()), nullptr, 0);
    if (wideLen <= 0) return;

    std::wstring wideUrl(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Url.data(),
                        static_cast<int>(utf8Url.size()), wideUrl.data(), wideLen);

    ShellExecuteW(nullptr, L"open", wideUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// ------------------------- RAII 进程句柄封装 -------------------------
class ProcessHandle {
public:
    explicit ProcessHandle(HANDLE h) noexcept : handle_(h) {}
    ~ProcessHandle() { if (handle_) CloseHandle(handle_); }
    ProcessHandle(const ProcessHandle&) = delete;
    ProcessHandle& operator=(const ProcessHandle&) = delete;
    ProcessHandle(ProcessHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    ProcessHandle& operator=(ProcessHandle&& other) noexcept {
        if (this != &other) {
            if (handle_) CloseHandle(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    HANDLE get() const noexcept { return handle_; }
private:
    HANDLE handle_;
};

// ------------------------- 运行外部处理程序 -------------------------
[[nodiscard]] bool runSongProcessor() {
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpFile = PROCESSOR_EXE.data();
    sei.nShow  = SW_SHOWNORMAL;
    sei.fMask  = SEE_MASK_NOCLOSEPROCESS;

    if (!ShellExecuteExA(&sei)) {
        std::cerr << "错误: 无法启动 " << PROCESSOR_EXE << "，请检查文件是否存在\n";
        return false;
    }

    if (sei.hProcess) {
        ProcessHandle ph(sei.hProcess);
        WaitForSingleObject(ph.get(), INFINITE);
    }
    return true;
}

// ------------------------- 从文件加载歌曲列表 -------------------------
[[nodiscard]] std::vector<std::string> loadSongsFromFile(std::string_view filename) {
    std::vector<std::string> songs;
    std::ifstream inFile(filename.data());
    if (!inFile) {
        std::cerr << "错误: 无法打开 " << filename << "，可能 SongProcessing.exe 执行失败\n";
        return songs;
    }

    std::string line;
    // 跳过前两行（表头及空行）
    if (!std::getline(inFile, line) || !std::getline(inFile, line)) {
        std::cerr << "警告: " << filename << " 格式异常，可能缺少表头\n";
    }

    while (std::getline(inFile, line)) {
        if (isStatisticsLine(line)) break;
        if (!line.empty()) songs.push_back(line);
    }
    return songs;
}

// ------------------------- 随机选择一首歌曲 -------------------------
[[nodiscard]] std::string pickRandomSong(const std::vector<std::string>& songs) {
    // 使用 thread_local 生成器避免每次重新初始化随机设备
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<size_t> dist(0, songs.size() - 1);
    return songs[dist(gen)];
}

// ------------------------- 主程序入口 -------------------------
int main() {
    SetConsoleOutputCP(CP_UTF8);

    // 先运行 SongProcessing.exe 更新并标准化歌曲列表
    if (!runSongProcessor()) return 1;

    auto songs = loadSongsFromFile(INPUT_FILE);
    if (songs.empty()) {
        std::cerr << "错误: 未读取到任何歌曲，无法随机选择\n";
        return 1;
    }

    std::string selected = pickRandomSong(songs);
    std::string url      = std::string(SEARCH_URL_TMPL) + urlEncode(selected);
    openUrlInBrowser(url);

    std::cout << "本次随机歌曲: " << selected << '\n';
    return 0;
}