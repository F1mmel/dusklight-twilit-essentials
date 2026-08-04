#include "update_service.hpp"

#include <windows.h>
#include <wininet.h>

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#pragma comment(lib, "wininet.lib")

bool g_configCheckForUpdatesEnabled = true;

static const LogService* s_logSvc = nullptr;
static ModContext* s_modCtx = nullptr;
static const UiService* s_uiSvc = nullptr;
static const ConfigService* s_configSvc = nullptr;
static ConfigVarHandle s_varCheckForUpdates = 0;

static const char* MOD_CURRENT_VERSION = "1.0.4";
static const char* GITHUB_API_URL = "https://api.github.com/repos/F1mmel/dusklight-twilit-essentials/releases/latest";

enum DownloadState {
    DL_IDLE = 0,
    DL_IN_PROGRESS,
    DL_SUCCESS,
    DL_FAILED,
    DL_HANDLED
};

static std::mutex s_mutex;
static bool s_updateAvailable = false;
static bool s_dialogShown = false;
static std::string s_latestTagName;
static std::string s_downloadUrl;

static std::atomic<DownloadState> s_downloadState{DL_IDLE};
static UiDialogHandle s_activeDialogHandle = 0;

static std::string http_get(const std::string& url) {
    std::string response;
    HINTERNET hInternet = InternetOpenA("TwilitEssentialsUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return response;
    }

    DWORD timeout = 5000; // 5s timeout
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        "User-Agent: TwilitEssentialsModUpdater\r\nAccept: application/vnd.github.v3+json",
        -1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
        0
    );

    if (hConnect) {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response.append(buffer, bytesRead);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hInternet);
    return response;
}

static bool download_file(const std::string& url, const std::string& destPath) {
    HINTERNET hInternet = InternetOpenA("TwilitEssentialsUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }

    DWORD timeout = 15000; // 15s download timeout
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        "User-Agent: TwilitEssentialsModUpdater",
        -1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
        0
    );

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    FILE* f = std::fopen(destPath.c_str(), "wb");
    if (!f) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead = 0;
    bool success = true;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (std::fwrite(buffer, 1, bytesRead, f) != bytesRead) {
            success = false;
            break;
        }
    }

    std::fclose(f);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return success;
}

static bool parse_github_release_json(const std::string& json, std::string& outTagName, std::string& outDownloadUrl) {
    outTagName.clear();
    outDownloadUrl.clear();

    size_t tagPos = json.find("\"tag_name\"");
    if (tagPos != std::string::npos) {
        size_t colonPos = json.find(':', tagPos);
        if (colonPos != std::string::npos) {
            size_t openQuote = json.find('"', colonPos);
            if (openQuote != std::string::npos) {
                size_t closeQuote = json.find('"', openQuote + 1);
                if (closeQuote != std::string::npos) {
                    outTagName = json.substr(openQuote + 1, closeQuote - openQuote - 1);
                }
            }
        }
    }

    size_t urlPos = 0;
    while ((urlPos = json.find("\"browser_download_url\"", urlPos)) != std::string::npos) {
        size_t colonPos = json.find(':', urlPos);
        if (colonPos != std::string::npos) {
            size_t openQuote = json.find('"', colonPos);
            if (openQuote != std::string::npos) {
                size_t closeQuote = json.find('"', openQuote + 1);
                if (closeQuote != std::string::npos) {
                    std::string url = json.substr(openQuote + 1, closeQuote - openQuote - 1);
                    if (url.find(".dusk") != std::string::npos || outDownloadUrl.empty()) {
                        outDownloadUrl = url;
                    }
                }
            }
        }
        urlPos += 22;
    }

    return !outTagName.empty() && !outDownloadUrl.empty();
}

static bool is_version_newer(const std::string& latestTag, const std::string& currentVersion) {
    std::string latest = latestTag;
    if (!latest.empty() && (latest[0] == 'v' || latest[0] == 'V')) {
        latest = latest.substr(1);
    }
    std::string current = currentVersion;
    if (!current.empty() && (current[0] == 'v' || current[0] == 'V')) {
        current = current.substr(1);
    }

    int lMaj = 0, lMin = 0, lPat = 0;
    int cMaj = 0, cMin = 0, cPat = 0;

    std::sscanf(latest.c_str(), "%d.%d.%d", &lMaj, &lMin, &lPat);
    std::sscanf(current.c_str(), "%d.%d.%d", &cMaj, &cMin, &cPat);

    if (lMaj > cMaj) return true;
    if (lMaj < cMaj) return false;
    if (lMin > cMin) return true;
    if (lMin < cMin) return false;
    return lPat > cPat;
}

static std::vector<std::string> get_target_mod_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32
    char appData[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appData, MAX_PATH) > 0) {
        std::string modPath = std::string(appData) + "\\TwilitRealm\\Dusklight\\mods\\dusklight_twilit_essentials.dusk";
        paths.push_back(modPath);
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
#if defined(__APPLE__)
        std::string modPath = std::string(home) + "/Library/Application Support/TwilitRealm/Dusklight/mods/dusklight_twilit_essentials.dusk";
        paths.push_back(modPath);
#else
        std::string modPath = std::string(home) + "/.local/share/TwilitRealm/Dusklight/mods/dusklight_twilit_essentials.dusk";
        paths.push_back(modPath);
#endif
    }
#endif

    return paths;
}

static void on_update_confirmed(ModContext*, UiDialogHandle, void*) {
    std::string downloadUrl;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        downloadUrl = s_downloadUrl;
    }

    if (downloadUrl.empty()) {
        s_downloadState = DL_FAILED;
        return;
    }

    s_downloadState = DL_IN_PROGRESS;

    std::thread([downloadUrl]() {
        auto paths = get_target_mod_paths();
        bool success = false;
        for (const auto& path : paths) {
            if (download_file(downloadUrl, path)) {
                success = true;
            }
        }
        if (success) {
            s_downloadState = DL_SUCCESS;
        } else {
            s_downloadState = DL_FAILED;
        }
    }).detach();
}

static void start_version_check_thread() {
    std::thread([]() {
        std::string json = http_get(GITHUB_API_URL);
        if (json.empty()) {
            return;
        }

        std::string tagName, downloadUrl;
        if (parse_github_release_json(json, tagName, downloadUrl)) {
            if (is_version_newer(tagName, MOD_CURRENT_VERSION)) {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_latestTagName = tagName;
                s_downloadUrl = downloadUrl;
                s_updateAvailable = true;

                if (s_logSvc && s_modCtx) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[Updater] Update available: %s (Current: %s)", tagName.c_str(), MOD_CURRENT_VERSION);
                    s_logSvc->info(s_modCtx, buf);
                }
            }
        }
    }).detach();
}

ModResult init_update_service(const LogService* log_svc, ModContext* mod_ctx, const UiService* ui_svc, const ConfigService* config_svc, ConfigVarHandle var_handle) {
    s_logSvc = log_svc;
    s_modCtx = mod_ctx;
    s_uiSvc = ui_svc;
    s_configSvc = config_svc;
    s_varCheckForUpdates = var_handle;

    s_updateAvailable = false;
    s_dialogShown = false;
    s_downloadState = DL_IDLE;

    if (g_configCheckForUpdatesEnabled) {
        start_version_check_thread();
    }

    return MOD_OK;
}

void update_update_service(const LogService*, ModContext* mod_ctx, const UiService* ui_svc) {
    if (!g_configCheckForUpdatesEnabled || !ui_svc) {
        return;
    }

    // 1. Show update prompt dialog when update is detected
    if (s_updateAvailable && !s_dialogShown) {
        s_dialogShown = true;

        std::string latestTag;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            latestTag = s_latestTagName;
        }

        static std::string s_promptRml;
        s_promptRml = "A new version of <b style=\"color: #d278ff;\">Twilit Essentials</b> is available on GitHub!<br/><br/>"
                      "Current Version: <b>v" + std::string(MOD_CURRENT_VERSION) + "</b><br/>"
                      "Latest Release: <b>" + latestTag + "</b><br/><br/>"
                      "Would you like to automatically download and update the mod file now?";

        static UiDialogAction s_actions[2];
        s_actions[0].label = "Yes";
        s_actions[0].on_pressed = on_update_confirmed;
        s_actions[0].user_data = nullptr;
        s_actions[0].keep_open = false;

        s_actions[1].label = "No";
        s_actions[1].on_pressed = nullptr;
        s_actions[1].user_data = nullptr;
        s_actions[1].keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Mod Update Available";
        desc.body_rml = s_promptRml.c_str();
        desc.variant = UI_DIALOG_NORMAL;
        desc.icon = "question-mark";
        desc.actions = s_actions;
        desc.action_count = 2;

        ui_svc->dialog_push(mod_ctx, &desc, &s_activeDialogHandle);
    }

    // 2. Handle completion of file download on main thread
    DownloadState state = s_downloadState.load();
    if (state == DL_SUCCESS) {
        s_downloadState = DL_HANDLED;

        std::string latestTag;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            latestTag = s_latestTagName;
        }

        static std::string s_successRml;
        s_successRml = "<b style=\"color: #d278ff;\">Twilit Essentials</b> has been updated to <b>" + latestTag + "</b>!<br/><br/>"
                       "The mod file was successfully replaced.<br/>"
                       "Please restart Dusklight to apply the update.";

        static UiDialogAction s_okAction;
        s_okAction.label = "OK";
        s_okAction.on_pressed = nullptr;
        s_okAction.user_data = nullptr;
        s_okAction.keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Update Complete";
        desc.body_rml = s_successRml.c_str();
        desc.variant = UI_DIALOG_NORMAL;
        desc.icon = "celebration";
        desc.actions = &s_okAction;
        desc.action_count = 1;

        UiDialogHandle hDialog = 0;
        ui_svc->dialog_push(mod_ctx, &desc, &hDialog);
    } else if (state == DL_FAILED) {
        s_downloadState = DL_HANDLED;

        static std::string s_failRml;
        s_failRml = "Failed to download the update package from GitHub.<br/>"
                    "Please check your internet connection or update manually.";

        static UiDialogAction s_okAction;
        s_okAction.label = "OK";
        s_okAction.on_pressed = nullptr;
        s_okAction.user_data = nullptr;
        s_okAction.keep_open = false;

        UiDialogDesc desc = UI_DIALOG_DESC_INIT;
        desc.title = "Update Failed";
        desc.body_rml = s_failRml.c_str();
        desc.variant = UI_DIALOG_DANGER;
        desc.icon = "error";
        desc.actions = &s_okAction;
        desc.action_count = 1;

        UiDialogHandle hDialog = 0;
        ui_svc->dialog_push(mod_ctx, &desc, &hDialog);
    }
}

void shutdown_update_service() {
}
