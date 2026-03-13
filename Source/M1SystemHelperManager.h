#pragma once

#include <string>
#include <set>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace Mach1 {

class M1SystemHelperManager
{
public:
    static M1SystemHelperManager& getInstance();

    bool requestHelperService(const std::string& appName);
    void releaseHelperService(const std::string& appName);
    bool isHelperServiceRunning() const;
    bool startHelperService();
    bool stopHelperService();

    int getActiveInstanceCount() const { return static_cast<int>(m_activeInstances.size()); }

    std::vector<std::string> getActiveInstances() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<std::string>(m_activeInstances.begin(), m_activeInstances.end());
    }

private:
    M1SystemHelperManager() = default;
    ~M1SystemHelperManager() = default;

    M1SystemHelperManager(const M1SystemHelperManager&) = delete;
    M1SystemHelperManager& operator=(const M1SystemHelperManager&) = delete;

#ifdef __APPLE__
    bool executeLaunchctlCommand(const std::string& command) const;
    bool isServiceLoaded() const;
    bool triggerSocketActivation() const;
    static const char* const SERVICE_LABEL;
    static const char* const SOCKET_PATH;
    static const char* const PLIST_PATH;
#elif defined(_WIN32)
    bool executeServiceCommand(const std::string& command) const;
    bool isServiceInstalled() const;
    bool triggerNamedPipeActivation() const;
    static const char* const SERVICE_NAME;
    static const char* const PIPE_NAME;
#else
    bool executeSystemctlCommand(const std::string& command) const;
    bool isServiceEnabled() const;
    bool triggerSocketActivation() const;
    static const char* const SERVICE_NAME;
    static const char* const SOCKET_PATH;
#endif

    mutable std::mutex m_mutex;
    std::set<std::string> m_activeInstances;
};

} // namespace Mach1
