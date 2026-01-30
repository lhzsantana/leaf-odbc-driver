#include "leafodbc/leaf_client.h"
#include "leafodbc/common.h"
#include <curl/curl.h>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace leafodbc {

struct WriteCallbackData {
    std::string* buffer;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    WriteCallbackData* data = static_cast<WriteCallbackData*>(userp);
    size_t total_size = size * nmemb;
    data->buffer->append(static_cast<char*>(contents), total_size);
    return total_size;
}

LeafClient::LeafClient(const std::string& endpoint_base, const std::string& user_agent,
                       int timeout_sec, bool verify_tls)
    : endpoint_base_(endpoint_base), user_agent_(user_agent), 
      timeout_sec_(timeout_sec), verify_tls_(verify_tls) {
}

std::string LeafClient::build_url(const std::string& path) const {
    std::string url = endpoint_base_;
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }
    if (!path.empty() && path[0] == '/') {
        url += path.substr(1);
    } else {
        url += path;
    }
    return url;
}

std::string LeafClient::escape_json_string(const std::string& str) const {
    std::ostringstream o;
    for (size_t i = 0; i < str.length(); ++i) {
        switch (str[i]) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= str[i] && str[i] <= '\x1f') {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                      << static_cast<int>(str[i]);
                } else {
                    o << str[i];
                }
        }
    }
    return o.str();
}

bool LeafClient::http_post(const std::string& url, const std::string& body,
                          const std::vector<std::string>& headers, std::string& response, int& status_code) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        log("Failed to initialize CURL");
        return false;
    }
    
    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    
    WriteCallbackData callback_data;
    callback_data.buffer = &response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec_);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
    
    if (!verify_tls_) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    } else {
        log("CURL error: " + std::string(curl_easy_strerror(res)));
        status_code = 0;
    }
    
    curl_slist_free_all(header_list);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK);
}

bool LeafClient::authenticate(const std::string& username, const std::string& password, bool remember_me) {
    std::string url = build_url("/api/authenticate");
    
    nlohmann::json auth_json;
    auth_json["username"] = username;
    auth_json["password"] = password;
    auth_json["rememberMe"] = remember_me;
    
    std::string body = auth_json.dump();
    std::vector<std::string> headers = {
        "Content-Type: application/json"
    };
    
    std::string response;
    int status_code = 0;
    
    if (should_log()) {
        log("Authenticating to " + url);
    }
    
    if (!http_post(url, body, headers, response, status_code)) {
        log("Authentication HTTP request failed");
        return false;
    }
    
    if (status_code != 200) {
        log("Authentication failed with status " + std::to_string(status_code) + ": " + response);
        return false;
    }
    
    try {
        nlohmann::json result = nlohmann::json::parse(response);
        if (result.contains("id_token")) {
            auth_token_ = result["id_token"].get<std::string>();
            if (should_log()) {
                log("Authentication successful, token obtained");
            }
            return true;
        } else {
            log("Authentication response missing id_token");
            return false;
        }
    } catch (const std::exception& e) {
        log("Failed to parse authentication response: " + std::string(e.what()));
        return false;
    }
}

bool LeafClient::execute_query(const std::string& sql, const std::string& sql_engine,
                               nlohmann::json& result) {
    if (auth_token_.empty()) {
        log("Not authenticated");
        return false;
    }
    
    std::string url = build_url("/services/pointlake/api/v2/query");
    url += "?sqlEngine=" + sql_engine;
    
    std::vector<std::string> headers = {
        "Authorization: Bearer " + auth_token_,
        "Content-Type: text/plain"
    };
    
    std::string response;
    int status_code = 0;
    
    if (should_log()) {
        log("Executing query: " + sql.substr(0, std::min(sql.length(), size_t(100))) + "...");
    }
    
    if (!http_post(url, sql, headers, response, status_code)) {
        log("Query HTTP request failed");
        return false;
    }
    
    if (status_code == 401) {
        log("Query returned 401 (unauthorized), token may be expired");
        return false;
    }
    
    if (status_code != 200) {
        log("Query failed with status " + std::to_string(status_code) + ": " + response);
        return false;
    }
    
    try {
        result = nlohmann::json::parse(response);
        
        // Handle two response formats:
        // A) Array: [{"colA": 1, ...}, ...]
        // B) Object with "rows": {"rows": [{"colA": 1, ...}, ...]}
        if (result.is_array()) {
            // Already in correct format
        } else if (result.is_object() && result.contains("rows")) {
            if (result["rows"].is_array()) {
                result = result["rows"];
            } else if (result["rows"].is_object() && result["rows"].contains("rows")) {
                result = result["rows"]["rows"];
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        log("Failed to parse query response: " + std::string(e.what()));
        return false;
    }
}

} // namespace leafodbc
