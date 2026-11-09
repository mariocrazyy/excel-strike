#pragma once

#include "http/HttpHelper.hpp"

struct ReleaseInfo {
    std::string tag;
    std::string executable_url;
    std::string checksum_url;
};

struct Status {
    std::string current_version;
    std::string latest_version;
    bool update_check_succeeded = false;
    std::string notice_title;
    std::string notice;
};

class Updater {
public:
    ~Updater() = default;
    Updater(const Updater&) = delete;
    Updater(Updater&&) = delete;
    Updater& operator=(const Updater&) = delete;
    Updater& operator=(Updater&&) = delete;

    static bool Init();
    static bool Update();
    static bool Process();
    static int HandleUpdateCommandLine();

    static Status GetStatus();
private:
    Updater() {};

    static Updater& GetInstance()
    {
        static Updater i{};
        return i;
    }

    bool InitImpl();
    bool UpdateImpl();
    bool ProcessImpl();
private:
    ReleaseInfo release;
    Status status;
    bool isSetup = false;
    std::string current_version = "v1.0.0";
    std::string release_url = "https://api.github.com/repos/mariocrazyy/excel-strike/releases/latest";
};
