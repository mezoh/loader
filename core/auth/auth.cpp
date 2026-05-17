#include "auth.h"

#include <Windows.h>
#include <winhttp.h>

#include <atomic>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include <core/auth/keyauth/json.hpp>
#include <core/crypt/lazy_importer.hpp>
#include <core/crypt/skCrypter.h>
#include <core/auth/hwid/hwid.h>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

namespace
{
    constexpr const char* kClientVersion = "0.1.0";
    constexpr const char* kClientBuild = __DATE__ " " __TIME__;
    constexpr const char* kDeviceName = "windows-loader";
}

static SecureString bearer_token;
static SecureString last_error;
static std::atomic<auth::HwidStatus> s_hwid_status{auth::HwidStatus::Ok};
static std::mutex auth_state_mutex;

#ifdef DISTORTION_API_LOCAL
#define API_HOST _(L"localhost")
#define API_PORT 8000
#define API_FLAGS 0
#else
#define API_HOST _(L"distortion.vip")
#define API_PORT INTERNET_DEFAULT_HTTPS_PORT
#define API_FLAGS WINHTTP_FLAG_SECURE
#endif

static void wipe_string(std::string& s)
{
    if (!s.empty()) {
        SecureZeroMemory(s.data(), s.size());
        s.clear();
    }
}

static void set_last_error(const std::string& value)
{
    std::scoped_lock lock(auth_state_mutex);
    last_error.wipe();
    last_error.set(value);
}

static void set_token(const std::string& value)
{
    std::scoped_lock lock(auth_state_mutex);
    bearer_token.wipe();
    bearer_token.set(value);
}

static std::string get_token_copy()
{
    std::scoped_lock lock(auth_state_mutex);
    return bearer_token.get();
}

static std::string make_http_error(DWORD status, const std::string& response)
{
    std::string message = _("HTTP ").decrypt() + std::to_string(status);
    if (!response.empty()) {
        std::string snippet = response.substr(0, 180);
        for (char& ch : snippet) {
            if (ch == '\r' || ch == '\n' || ch == '\t') {
                ch = ' ';
            }
        }
        message += _(": ").decrypt() + snippet;
    }
    return message;
}

static bool http_request(LPCWSTR method, LPCWSTR path, const std::wstring& headers, const std::string& body,
                         std::string& response_out, DWORD& status_out)
{
    static HMODULE winhttp_mod = LI_FN(LoadLibraryW)(_(L"winhttp.dll").decrypt());
    if (!winhttp_mod) {
        return false;
    }

    constexpr LPCWSTR k_no_proxy_name = nullptr;
    constexpr LPCWSTR k_no_proxy_bypass = nullptr;
    constexpr LPCWSTR k_version = nullptr;
    constexpr LPCWSTR k_referrer = nullptr;
    constexpr LPCWSTR k_header_name = nullptr;
    constexpr LPDWORD k_header_index = nullptr;
    LPCWSTR* const accept_types = nullptr;

    HINTERNET session = LI_FN(WinHttpOpen)
                            .in(winhttp_mod)(_(L"DistortionLoader/1.0").decrypt(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             k_no_proxy_name, k_no_proxy_bypass, 0);
    if (!session) {
        return false;
    }

    HINTERNET connection = LI_FN(WinHttpConnect).in(winhttp_mod)(session, API_HOST.decrypt(), API_PORT, 0);
    if (!connection) {
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(session);
        return false;
    }

    HINTERNET request = LI_FN(WinHttpOpenRequest)
                            .in(winhttp_mod)(connection, method, path, k_version, k_referrer, accept_types, API_FLAGS);
    if (!request) {
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(connection);
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(session);
        return false;
    }

    LI_FN(WinHttpSetTimeouts).in(winhttp_mod)(request, 5000, 5000, 10000, 10000);

    LPCWSTR hdr_ptr = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
    DWORD hdr_len = headers.empty() ? 0 : (DWORD)-1;
    DWORD body_len = (DWORD)body.size();

    BOOL sent = LI_FN(WinHttpSendRequest)
                    .in(winhttp_mod)(request, hdr_ptr, hdr_len, body.empty() ? nullptr : (void*)body.c_str(), body_len,
                                     body_len, 0);

    if (!sent || !LI_FN(WinHttpReceiveResponse).in(winhttp_mod)(request, nullptr)) {
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(request);
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(connection);
        LI_FN(WinHttpCloseHandle).in(winhttp_mod)(session);
        return false;
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    LI_FN(WinHttpQueryHeaders)
        .in(winhttp_mod)(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, k_header_name, &status, &size,
                         k_header_index);
    status_out = status;

    DWORD available = 0;
    DWORD read = 0;
    while (LI_FN(WinHttpQueryDataAvailable).in(winhttp_mod)(request, &available) && available > 0) {
        std::string chunk(available, '\0');
        LI_FN(WinHttpReadData).in(winhttp_mod)(request, &chunk[0], available, &read);
        response_out.append(chunk, 0, read);
    }

    LI_FN(WinHttpCloseHandle).in(winhttp_mod)(request);
    LI_FN(WinHttpCloseHandle).in(winhttp_mod)(connection);
    LI_FN(WinHttpCloseHandle).in(winhttp_mod)(session);
    return true;
}

static std::wstring json_headers()
{
    return _(L"Content-Type: application/json\r\n").decrypt();
}

static std::wstring auth_headers()
{
    std::string token = get_token_copy();
    std::wstring headers = std::wstring(_(L"Authorization: Bearer ").decrypt()) +
                           std::wstring(token.begin(), token.end()) + _(L"\r\n").decrypt();
    wipe_string(token);
    return headers;
}

static std::wstring auth_json_headers()
{
    return json_headers() + auth_headers();
}

static bool parse_json_response(const std::string& response, json& parsed)
{
    try {
        parsed = json::parse(response);
        return true;
    }
    catch (...) {
        return false;
    }
}

static std::string json_string(const json& value, const char* key)
{
    if (!value.is_object() || !value.contains(key) || !value[key].is_string()) {
        return {};
    }

    return value[key].get<std::string>();
}

static bool json_bool(const json& value, const char* key, bool fallback = false)
{
    if (!value.is_object() || !value.contains(key) || !value[key].is_boolean()) {
        return fallback;
    }

    return value[key].get<bool>();
}

static void populate_session_from_json(const json& payload, auth::SessionResult& out)
{
    out = {};

    const json& user = payload.contains("user") ? payload["user"] : json::object();
    const json& subscription = payload.contains("subscription") ? payload["subscription"] : json::object();
    const json& loader = payload.contains("loader") ? payload["loader"] : json::object();

    out.username.set(json_string(user, "name"));

    auth::Subscription sub;
    sub.game.set(json_string(subscription, "game"));
    sub.tier.set(json_string(subscription, "plan"));
    sub.expires_at.set(json_string(subscription, "expires_at"));
    sub.updated_at.set(json_string(loader, "published_at"));

    if (!sub.game.empty()) {
        out.subscriptions.push_back(std::move(sub));
    }

    out.latest_version.set(json_string(loader, "latest_version"));
    out.required_version.set(json_string(loader, "required_version"));
    out.loader_message.set(json_string(loader, "message"));
    out.update_available = json_bool(loader, "update_available");
    out.force_redownload = json_bool(loader, "force_redownload");
}

bool auth::Subscription::is_active() const
{
    if (expires_at.empty()) {
        return false;
    }

    const std::string exp = expires_at.get();
    std::tm tm = {};
    std::istringstream ss(exp);
    ss >> std::get_time(&tm, _("%Y-%m-%dT%H:%M:%S").decrypt());
    if (ss.fail()) {
        return false;
    }

    return time(nullptr) < _mkgmtime(&tm);
}

bool auth::login(const std::string& username, const std::string& password)
{
    set_last_error("");
    set_token("");
    s_hwid_status = auth::HwidStatus::Ok;

    std::string hwid_hash = hwid::generate();
    if (hwid_hash.empty()) {
        set_last_error(_("Failed to generate hardware fingerprint.").decrypt());
        s_hwid_status = auth::HwidStatus::Error;
        return false;
    }

    json body = {
        {"name", username},
        {"password", password},
        {"version", kClientVersion},
        {"build", kClientBuild},
        {"device_name", kDeviceName},
        {"hwid", hwid_hash},
    };
    wipe_string(hwid_hash);

    std::string response;
    DWORD status = 0;

    std::string body_str = body.dump();
    bool request_ok = http_request(_(L"POST").decrypt(), _(L"/api/loader/auth/login").decrypt(),
                                    json_headers(), body_str, response, status);
    wipe_string(body_str);

    if (!request_ok) {
        set_last_error(_("Login request failed before the API responded.").decrypt());
        return false;
    }

    json payload;
    if (!parse_json_response(response, payload)) {
        set_last_error(_("Login response was not valid JSON.").decrypt());
        wipe_string(response);
        return false;
    }

    if (status == 423) {
        std::string code = json_string(payload, "code");
        if (code == _("hwid_mismatch").decrypt()) {
            s_hwid_status = auth::HwidStatus::Mismatch;
            std::string message = json_string(payload, "message");
            if (message.empty()) {
                message = _("HWID mismatch. Visit distortion.vip to request a reset.").decrypt();
            }
            set_last_error(message);
            wipe_string(response);
            return false;
        }
    }

    if (status != 200) {
        std::string message = json_string(payload, "message");
        if (message.empty()) {
            message = _("Login failed with ").decrypt() + make_http_error(status, response);
        }
        set_last_error(message);
        wipe_string(response);
        return false;
    }

    std::string token = json_string(payload, "token");
    if (token.empty()) {
        set_last_error(_("Login succeeded but no loader token was returned.").decrypt());
        wipe_string(response);
        return false;
    }

    set_token(token);
    wipe_string(token);
    wipe_string(response);
    return true;
}

bool auth::check_session(SessionResult& out)
{
    set_last_error("");

    std::string response;
    DWORD status = 0;

    if (!http_request(_(L"GET").decrypt(), _(L"/api/loader/auth/session").decrypt(), auth_headers(), "", response,
                      status)) {
        set_last_error(_("Session request failed before the API responded.").decrypt());
        return false;
    }

    json payload;
    if (!parse_json_response(response, payload)) {
        set_last_error(_("Session response was not valid JSON.").decrypt());
        wipe_string(response);
        return false;
    }

    if (status != 200) {
        std::string message = json_string(payload, "message");
        if (message.empty()) {
            message = _("Session lookup failed with ").decrypt() + make_http_error(status, response);
        }
        set_last_error(message);
        wipe_string(response);
        return false;
    }

    populate_session_from_json(payload, out);
    wipe_string(response);
    return true;
}

bool auth::heartbeat(SessionResult* out)
{
    set_last_error("");

    json body = {
        {"version", kClientVersion},
        {"build", kClientBuild},
    };

    std::string response;
    DWORD status = 0;

    if (!http_request(_(L"POST").decrypt(), _(L"/api/loader/auth/heartbeat").decrypt(), auth_json_headers(),
                      body.dump(), response, status)) {
        set_last_error(_("Heartbeat request failed before the API responded.").decrypt());
        return false;
    }

    json payload;
    if (!parse_json_response(response, payload)) {
        set_last_error(_("Heartbeat response was not valid JSON.").decrypt());
        wipe_string(response);
        return false;
    }

    if (status != 200) {
        std::string message = json_string(payload, "message");
        if (message.empty()) {
            message = _("Heartbeat failed with ").decrypt() + make_http_error(status, response);
        }
        set_last_error(message);
        wipe_string(response);
        return false;
    }

    if (out) {
        populate_session_from_json(payload, *out);
    }

    wipe_string(response);
    return true;
}

bool auth::logout()
{
    set_last_error("");

    std::string response;
    DWORD status = 0;

    if (!http_request(_(L"POST").decrypt(), _(L"/api/loader/auth/logout").decrypt(), auth_headers(), "", response,
                      status)) {
        set_last_error(_("Logout request failed before the API responded.").decrypt());
        return false;
    }

    if (status != 200) {
        json payload;
        std::string message;
        if (parse_json_response(response, payload)) {
            message = json_string(payload, "message");
        }

        if (message.empty()) {
            message = _("Logout failed with ").decrypt() + make_http_error(status, response);
        }

        set_last_error(message);
        wipe_string(response);
        return false;
    }

    set_token("");
    wipe_string(response);
    return true;
}

auth::HwidStatus auth::check_hwid()
{
    return s_hwid_status;
}

std::string auth::get_token()
{
    return get_token_copy();
}

std::string auth::get_last_error()
{
    std::scoped_lock lock(auth_state_mutex);
    return last_error.get();
}
