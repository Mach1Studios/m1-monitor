#include "M1SystemHelperManager.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cstring>

#ifdef __APPLE__
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <winsvc.h>
    #include <tchar.h>
#else
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
#endif

namespace Mach1 {

namespace
{
#ifdef __APPLE__
bool pathExists(const char* path)
{
    return access(path, F_OK) == 0;
}

bool launchHelperApplicationDirectly()
{
    constexpr const char* appCandidates[] = {
        "/Library/Application Support/Mach1/m1-system-helper.app",
        "/Applications/Mach1 Spatial System/m1-system-helper.app",
    };

    for (const auto* path : appCandidates)
    {
        if (pathExists(path))
        {
            const std::string command = "open -g \"" + std::string(path) + "\" >/dev/null 2>&1";
            if (std::system(command.c_str()) == 0)
                return true;
        }
    }

    constexpr const char* binaryCandidates[] = {
        "/Library/Application Support/Mach1/m1-system-helper",
    };

    for (const auto* path : binaryCandidates)
    {
        if (pathExists(path))
        {
            const std::string command = "\"" + std::string(path) + "\" >/dev/null 2>&1 &";
            if (std::system(command.c_str()) == 0)
                return true;
        }
    }

    return false;
}
#elif defined(_WIN32)
bool launchHelperApplicationDirectly()
{
    constexpr const wchar_t* executableCandidates[] = {
        L"C:\\Program Files\\Mach1\\m1-system-helper.exe",
        L"C:\\ProgramData\\Mach1\\m1-system-helper.exe",
    };

    for (const auto* path : executableCandidates)
    {
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
            continue;

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo{};
        std::wstring commandLine = std::wstring(L"\"") + path + L"\"";
        if (CreateProcessW(path,
                           commandLine.data(),
                           NULL,
                           NULL,
                           FALSE,
                           DETACHED_PROCESS | CREATE_NO_WINDOW,
                           NULL,
                           NULL,
                           &startupInfo,
                           &processInfo))
        {
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            return true;
        }
    }

    return false;
}
#endif
}

#ifdef __APPLE__
const char* const M1SystemHelperManager::SERVICE_LABEL = "com.mach1.spatial.helper";
const char* const M1SystemHelperManager::SOCKET_PATH = "/tmp/com.mach1.spatial.helper.socket";
const char* const M1SystemHelperManager::PLIST_PATH = "/Library/LaunchAgents/com.mach1.spatial.helper.plist";
#elif defined(_WIN32)
const char* const M1SystemHelperManager::SERVICE_NAME = "M1-System-Helper";
const char* const M1SystemHelperManager::PIPE_NAME = "\\\\.\\pipe\\M1SystemHelper";
#else
const char* const M1SystemHelperManager::SERVICE_NAME = "m1-system-helper";
const char* const M1SystemHelperManager::SOCKET_PATH = "/tmp/m1-system-helper.socket";
#endif

M1SystemHelperManager& M1SystemHelperManager::getInstance()
{
    static M1SystemHelperManager instance;
    return instance;
}

bool M1SystemHelperManager::requestHelperService(const std::string& appName)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto insertResult = m_activeInstances.insert(appName);
    bool wasNewInstance = insertResult.second;

    std::cout << "[M1SystemHelperManager] " << appName << " requested service. "
              << "Active instances: " << m_activeInstances.size()
              << (wasNewInstance ? " (new instance)" : " (existing instance)") << std::endl;

    if (isHelperServiceRunning()) {
        std::cout << "[M1SystemHelperManager] Service already running for " << appName << std::endl;
        return true;
    }

    std::cout << "[M1SystemHelperManager] Starting helper service for " << appName << std::endl;

    if (!startHelperService()) {
        std::cerr << "[M1SystemHelperManager] Failed to start service" << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return true;
}

void M1SystemHelperManager::releaseHelperService(const std::string& appName)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_activeInstances.erase(appName);

    std::cout << "[M1SystemHelperManager] " << appName << " released helper service. "
              << "Active instances: " << m_activeInstances.size() << std::endl;

    if (m_activeInstances.empty()) {
        std::cout << "[M1SystemHelperManager] No more active instances. Service will auto-exit." << std::endl;
    }
}

bool M1SystemHelperManager::isHelperServiceRunning() const
{
#ifdef __APPLE__
    return triggerSocketActivation();
#elif defined(_WIN32)
    return triggerNamedPipeActivation();
#else
    return triggerSocketActivation();
#endif
}

bool M1SystemHelperManager::startHelperService()
{
#ifdef __APPLE__
    if (launchHelperApplicationDirectly()) {
        for (int attempt = 0; attempt < 8; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (triggerSocketActivation())
                return true;
        }
    }

    if (!isServiceLoaded()) {
        if (!executeLaunchctlCommand("load " + std::string(PLIST_PATH))) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    const bool startResult = executeLaunchctlCommand("start " + std::string(SERVICE_LABEL));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    bool connectionResult = triggerSocketActivation();
    if (!connectionResult && startResult) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        connectionResult = triggerSocketActivation();
    }

    return connectionResult;
#elif defined(_WIN32)
    if (launchHelperApplicationDirectly()) {
        for (int attempt = 0; attempt < 8; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (triggerNamedPipeActivation())
                return true;
        }
    }

    return executeServiceCommand("start");
#else
    if (!isServiceEnabled()) {
        if (!executeSystemctlCommand("enable " + std::string(SERVICE_NAME))) {
            return false;
        }
    }
    return executeSystemctlCommand("start " + std::string(SERVICE_NAME));
#endif
}

bool M1SystemHelperManager::stopHelperService()
{
#ifdef __APPLE__
    return executeLaunchctlCommand("stop " + std::string(SERVICE_LABEL));
#elif defined(_WIN32)
    return executeServiceCommand("stop");
#else
    return executeSystemctlCommand("stop " + std::string(SERVICE_NAME));
#endif
}

#ifdef __APPLE__

bool M1SystemHelperManager::executeLaunchctlCommand(const std::string& command) const
{
    const std::string fullCommand = "launchctl " + command + " 2>/dev/null";
    return std::system(fullCommand.c_str()) == 0;
}

bool M1SystemHelperManager::isServiceLoaded() const
{
    const std::string command = "launchctl list | grep " + std::string(SERVICE_LABEL) + " >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

bool M1SystemHelperManager::triggerSocketActivation() const
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bool success = false;
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        success = true;
        const char* ping = "PING\n";
        if (send(sockfd, ping, strlen(ping), 0) > 0) {
            char buffer[256] = {0};
            recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        }
    }

    close(sockfd);
    return success;
}

#endif

#ifdef _WIN32

bool M1SystemHelperManager::executeServiceCommand(const std::string& command) const
{
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scManager) {
        return false;
    }

    SC_HANDLE service = OpenService(scManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (!service) {
        CloseServiceHandle(scManager);
        return false;
    }

    bool success = false;
    if (command == "start") {
        success = StartService(service, 0, NULL);
        if (!success && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
            success = true;
        }
    } else if (command == "stop") {
        SERVICE_STATUS status;
        success = ControlService(service, SERVICE_CONTROL_STOP, &status);
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return success;
}

bool M1SystemHelperManager::isServiceInstalled() const
{
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scManager) {
        return false;
    }

    SC_HANDLE service = OpenService(scManager, SERVICE_NAME, SERVICE_QUERY_STATUS);
    const bool installed = (service != NULL);

    if (service) {
        CloseServiceHandle(service);
    }
    CloseServiceHandle(scManager);
    return installed;
}

bool M1SystemHelperManager::triggerNamedPipeActivation() const
{
    HANDLE pipe = CreateFile(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    const char* message = "PING";
    DWORD bytesWritten = 0;
    const bool success = WriteFile(pipe, message, static_cast<DWORD>(strlen(message)), &bytesWritten, NULL);

    if (success) {
        char buffer[256];
        DWORD bytesRead = 0;
        ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    }

    CloseHandle(pipe);
    return success;
}

#endif

#if defined(__linux__) && !defined(__APPLE__)

bool M1SystemHelperManager::executeSystemctlCommand(const std::string& command) const
{
    const std::string fullCommand = "systemctl " + command + " 2>/dev/null";
    return std::system(fullCommand.c_str()) == 0;
}

bool M1SystemHelperManager::isServiceEnabled() const
{
    const std::string command = "systemctl is-enabled " + std::string(SERVICE_NAME) + " >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

bool M1SystemHelperManager::triggerSocketActivation() const
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bool success = false;
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        success = true;
        const char* ping = "PING\n";
        send(sockfd, ping, strlen(ping), 0);
        char buffer[256];
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    }

    close(sockfd);
    return success;
}

#endif

} // namespace Mach1
