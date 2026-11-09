
#include "LogHelper.hpp"

#include <cstdio>
#include <mutex>

#define ADD_COLOR_TO_STREAM(color) "\x1b[" << int(color) << "m"
#define RESET_STREAM_COLOR "\x1b[0m"
#define HEX(value) "0x" << std::hex << std::uppercase << DWORD64(value) << std::dec << std::nouppercase

namespace {
    HICON application_icon = nullptr;
    std::mutex console_output_mutex;

    void WriteConsoleLog(const std::string& message)
    {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        if (output == nullptr || output == INVALID_HANDLE_VALUE) {
            OutputDebugStringA("Could not get the console output handle\n");
            return;
        }

        size_t offset = 0;
        while (offset < message.size()) {
            const DWORD chunk_size = static_cast<DWORD>(
                std::min(message.size() - offset, static_cast<size_t>(MAXDWORD)));
            DWORD bytes_written = 0;
            if (!WriteFile(output, message.data() + offset, chunk_size, &bytes_written, nullptr)) {
                OutputDebugStringA(std::format(
                    "Could not write application log to the console (Windows error {})\n",
                    GetLastError()).c_str());
                return;
            }
            if (bytes_written == 0) {
                OutputDebugStringA("Console output handle wrote no application log data\n");
                return;
            }
            offset += bytes_written;
        }
    }

    bool OpenFileLog(
        std::ofstream& file,
        std::filesystem::path& active_log_path,
        std::filesystem::path& executable_directory)
    {
        char executable_path[MAX_PATH]{};
        const DWORD path_length = GetModuleFileNameA(nullptr, executable_path, MAX_PATH);
        if (path_length == 0 || path_length >= MAX_PATH)
            return false;

        executable_directory = std::filesystem::path(executable_path).parent_path();
        active_log_path = executable_directory / "!latest.log";
        file.open(active_log_path, std::ios_base::out | std::ios_base::trunc);
        return file.is_open();
    }

    std::string FormatLogDate()
    {
        SYSTEMTIME local_time{};
        GetLocalTime(&local_time);
        return cfg::settings::log_date_format == cfg::LogDateFormat::AMERICAN
            ? std::format("{:02}-{:02}-{:04}", local_time.wMonth, local_time.wDay, local_time.wYear)
            : std::format("{:02}-{:02}-{:04}", local_time.wDay, local_time.wMonth, local_time.wYear);
    }

    bool ReserveArchivePath(const std::filesystem::path& log_directory, std::filesystem::path& archive_path)
    {
        const std::string date = FormatLogDate();

        for (unsigned int index = 1; ; ++index) {
            const auto destination = log_directory / std::format("{}_[{}].log", date, index);
            HANDLE file = CreateFileA(
                destination.string().c_str(),
                GENERIC_WRITE,
                0,
                nullptr,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (file != INVALID_HANDLE_VALUE) {
                CloseHandle(file);
                archive_path = destination;
                return true;
            }

            const DWORD error_code = GetLastError();
            if (error_code != ERROR_ALREADY_EXISTS && error_code != ERROR_FILE_EXISTS) {
                OutputDebugStringA(std::format(
                    "Could not reserve log archive path (Windows error {}): {}\n",
                    error_code,
                    destination.string()).c_str());
                return false;
            }
        }
    }

    void EnsureControllableConsole()
    {
        const HWND existing_console = GetConsoleWindow();
        if (existing_console && !FreeConsole()) {
            LOGF(WARNING, "Could not detach from the inherited console");
            return;
        }

        if (!AllocConsole()) {
            LOGF(WARNING, "Could not create a controllable console window");
            return;
        }

        const bool stdin_ready = freopen("CONIN$", "r", stdin) != nullptr;
        const bool stdout_ready = freopen("CONOUT$", "w", stdout) != nullptr;
        const bool stderr_ready = freopen("CONOUT$", "w", stderr) != nullptr;
        if (!stdin_ready || !stdout_ready || !stderr_ready)
            LOGF(WARNING, "Could not redirect all standard streams to the console window");

        std::cin.clear();
        std::cout.clear();
        std::cerr.clear();
    }

    HICON LoadApplicationIcon()
    {
        char executable_path[MAX_PATH]{};
        const DWORD path_length = GetModuleFileNameA(nullptr, executable_path, MAX_PATH);
        std::filesystem::path icon_path;
        HICON icon = nullptr;
        if (path_length != 0 && path_length < MAX_PATH) {
            icon_path = std::filesystem::path(executable_path).parent_path() / "excel.ico";
            icon = reinterpret_cast<HICON>(LoadImageA(
                nullptr,
                icon_path.string().c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE
            ));
        }
        else {
            LOGF(WARNING, "Could not resolve the executable path to load excel.ico");
        }

        if (!icon) {
            icon = reinterpret_cast<HICON>(LoadImageA(
                GetModuleHandleA(nullptr),
                MAKEINTRESOURCEA(1),
                IMAGE_ICON,
                0,
                0,
                LR_DEFAULTSIZE
            ));
        }

        if (!icon) {
            const HICON stock_icon = LoadIconA(nullptr, IDI_APPLICATION);
            if (stock_icon)
                icon = CopyIcon(stock_icon);
        }

        if (!icon) {
            LOGF(WARNING, "Could not load an application icon from {} or Windows resources", icon_path.string());
        }

        return icon;
    }

}

bool LogHelper::Init()
{
    return GetInstance().InitImpl();
}

void LogHelper::StartCrashArchive()
{
    GetInstance().StartCrashArchiveImpl();
}

void LogHelper::HideConsole()
{
    HWND console = GetConsoleWindow();
    if (!console)
        return;

    bool detached = false;
    {
        std::lock_guard lock(console_output_mutex);
        std::cout.flush();
        std::cerr.flush();
        detached = FreeConsole() == TRUE;
    }

    if (!detached)
        LOGF(WARNING, "Could not detach the application from its console");
    else {
        ShowWindow(console, SW_HIDE);
        PostMessageA(console, WM_CLOSE, 0, 0);
    }
}

void LogHelper::Destroy()
{
    Logger::FlushQueue();
    Logger::Destroy();
}

void LogHelper::StartCrashArchiveImpl()
{
    std::lock_guard lock(console_output_mutex);
    if (m_ActiveLogPath.empty() || !m_ArchiveLogPath.empty())
        return;

    const auto log_directory = m_ExecutableDirectory / "logs";
    std::error_code error;
    std::filesystem::create_directories(log_directory, error);
    if (error) {
        OutputDebugStringA(std::format("Could not create log archive directory: {}\n", error.message()).c_str());
        return;
    }

    if (!ReserveArchivePath(log_directory, m_ArchiveLogPath))
        return;

    m_CrashArchiveOut.open(m_ArchiveLogPath, std::ios_base::out | std::ios_base::app);
    if (!m_CrashArchiveOut.is_open()) {
        OutputDebugStringA(std::format("Could not open crash-recovery log: {}\n", m_ArchiveLogPath.string()).c_str());
        DeleteFileA(m_ArchiveLogPath.string().c_str());
        m_ArchiveLogPath.clear();
        return;
    }

    m_FileOut.flush();
    std::ifstream existing_log(m_ActiveLogPath, std::ios_base::in | std::ios_base::binary);
    if (existing_log.is_open()) {
        m_CrashArchiveOut << existing_log.rdbuf();
        m_CrashArchiveOut.flush();
        if (existing_log.bad() || !m_CrashArchiveOut) {
            OutputDebugStringA(std::format("Could not seed crash-recovery log: {}\n", m_ArchiveLogPath.string()).c_str());
            m_CrashArchiveOut.close();
            DeleteFileA(m_ArchiveLogPath.string().c_str());
            m_ArchiveLogPath.clear();
        }
    }
    if (m_CrashArchiveOut.is_open())
        LOGF(INFO, "Crash-recovery log: {}", m_ArchiveLogPath.string());
}

void LogHelper::Free() {
    auto& helper = GetInstance();
    if (HWND console = GetConsoleWindow()) {
        if (application_icon) {
            SendMessageA(console, WM_SETICON, ICON_BIG, 0);
            SendMessageA(console, WM_SETICON, ICON_SMALL, 0);
        }
        std::lock_guard lock(console_output_mutex);
        std::cout.flush();
        std::cerr.flush();
        FreeConsole();
    }
    {
        std::lock_guard lock(console_output_mutex);
        helper.m_FileOut.flush();
        helper.m_FileOut.close();
        helper.m_CrashArchiveOut.flush();
        helper.m_CrashArchiveOut.close();

        if (!helper.m_ActiveLogPath.empty() && !helper.m_ArchiveLogPath.empty() &&
            !MoveFileExA(
                helper.m_ActiveLogPath.string().c_str(),
                helper.m_ArchiveLogPath.string().c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            OutputDebugStringA(std::format(
                "Could not finalize log archive (Windows error {}): {}\n",
                GetLastError(),
                helper.m_ActiveLogPath.string()).c_str());
        }
    }
    if (application_icon) {
        DestroyIcon(application_icon);
        application_icon = nullptr;
    }
}

bool LogHelper::InitImpl() {
    Logger::Init();

    EnsureControllableConsole();
    application_icon = LoadApplicationIcon();

    if (auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
        handle != nullptr && handle != INVALID_HANDLE_VALUE)
    {
        if (!SetConsoleTitleA("Excel"))
            LOGF(WARNING, "Could not set the console title");

        if (HWND console = GetConsoleWindow(); console && application_icon) {
            SendMessageA(console, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(application_icon));
            SendMessageA(console, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(application_icon));
        }

        SetConsoleOutputCP(CP_UTF8);

        DWORD consoleMode{};
        if (GetConsoleMode(handle, &consoleMode)) {
            consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
            consoleMode &= ~(ENABLE_QUICK_EDIT_MODE);
            if (!SetConsoleMode(handle, consoleMode))
                LOGF(WARNING, "Could not enable console color support");
        }
        else {
            LOGF(WARNING, "Could not read the console output mode");
        }
    }

    const bool file_log_opened = OpenFileLog(m_FileOut, m_ActiveLogPath, m_ExecutableDirectory);

    Logger::AddSink([this](LogMessagePtr msg) {
#ifndef _DEBUG
        if (msg->Level() == eLogLevel::VERBOSE)
            return;
#endif
        std::string formatted = this->FormatConsole(msg);

        std::lock_guard lock(console_output_mutex);
        WriteConsoleLog(formatted);

        if (m_FileOut.is_open()) {
            m_FileOut << this->FormatFile(msg);
            m_FileOut.flush();
        }
        if (m_CrashArchiveOut.is_open()) {
            m_CrashArchiveOut << this->FormatFile(msg);
            m_CrashArchiveOut.flush();
        }
    });

    if (!file_log_opened)
        LOGF(WARNING, "Could not open a temporary application log beside the executable");
    if (file_log_opened)
        LOGF(INFO, "Session log: {}", m_ActiveLogPath.string());

    return true;
}

std::string LogHelper::FormatConsole(const LogMessagePtr msg) {
        std::stringstream out;

#ifdef _DEBUG
        const auto timestamp = std::format("{0:%H:%M:%S}", msg->Timestamp());
#else
        const auto timestamp = std::format("{0:%H:%M:%S}", std::chrono::floor<std::chrono::seconds>(msg->Timestamp()));
#endif
		const auto& location = msg->Location();
		const auto level     = msg->Level();
		const auto color     = GetColor(level);

		const auto file = std::filesystem::path(location.file_name()).filename().string();

#ifdef _DEBUG
		out << ADD_COLOR_TO_STREAM(LogColor::GRAY) << "[" << timestamp << "]" << ADD_COLOR_TO_STREAM(color) << "[" << GetLevelStr(level) << "/" << file << ":"
		    << location.line() << "] " << RESET_STREAM_COLOR << msg->Message();
#else
        out << ADD_COLOR_TO_STREAM(LogColor::GRAY) << "[" << timestamp << "]" << ADD_COLOR_TO_STREAM(color) << "[" << GetLevelStr(level) << "] " << RESET_STREAM_COLOR << msg->Message();
#endif

		return out.str();
}

std::string LogHelper::FormatFile(const LogMessagePtr msg) {
        std::stringstream out;
        const auto timestamp = std::format("{0:%Y-%m-%d %H:%M:%S}", std::chrono::floor<std::chrono::seconds>(msg->Timestamp()));
        const auto& location = msg->Location();
        const auto file = std::filesystem::path(location.file_name()).filename().string();

        out << "[" << timestamp << "][" << GetLevelStr(msg->Level()) << "][" << file << ":" << location.line() << "] "
            << msg->Message();

        return out.str();
}

LogColor LogHelper::GetColor(const eLogLevel level)
{
    switch (level)
    {
    case eLogLevel::VERBOSE: return LogColor::BLUE;
    case eLogLevel::INFO: return LogColor::GREEN;
    case eLogLevel::WARNING: return LogColor::YELLOW;
    case eLogLevel::FATAL: return LogColor::RED;
    }
    return LogColor::WHITE;
}

const char* LogHelper::GetLevelStr(const eLogLevel level) {
    constexpr std::array<const char*, 4> levelStrings = {{"DBG", "INF", "WRN", "FTL"}};

    return levelStrings[level];
}
