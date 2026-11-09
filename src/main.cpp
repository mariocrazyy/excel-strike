#include <iostream>

#include "updater/Updater.hpp"
#include "core/engine/Engine.hpp"
#include "gui/renderer/Renderer.hpp"

#include <external/exception.hpp>

int main()
{
    const int update_command_result = Updater::HandleUpdateCommandLine();
    if (update_command_result >= 0)
        return update_command_result;

    c_exception_handler::setup();

    LogHelper::Init();
    if (!Config::Read())
        LOGF(WARNING, "Failed to parse config, using default values");
    LogHelper::StartCrashArchive();

    LOGF(INFO, "Compiled {}, Welcome to Excel-Strike!", __TIMESTAMP__);
    LOGF(INFO, "Starting application initialization...");

    // Needs to be ran as ADMINISTRATOR
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        LOGF(WARNING, "Could not set application process priority to HIGH");

    LOGF(INFO, "Checking for GitHub release updates...");
    bool continue_startup = true;
    if (!Updater::Init())
        LOGF(WARNING, "Update check failed; continuing with the installed version");
    else
        continue_startup = Updater::Process();

    if (continue_startup) {
        LOGF(INFO, "Initializing game connection...");
        if (!Engine::Init()) {
            LOGF(FATAL, "Engine failed to initialize, cannot continue execution");
        }
        else if (!Renderer::Init()) {
            LOGF(FATAL, "Renderer failed to initialize, cannot continue execution");
        }
        else {
            LOGF(INFO, "Everything setup and ready, just... make sure you are not in \"Full Screen\"!");
            Renderer::Thread();
        }
    }

    LOGF(INFO, "Application shutdown started");
    LogHelper::Destroy();
    LogHelper::Free();
    return 0;
}