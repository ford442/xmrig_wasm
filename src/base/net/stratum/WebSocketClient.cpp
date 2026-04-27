/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "base/net/stratum/WebSocketClient.h"
#include "3rdparty/rapidjson/document.h"
#include "3rdparty/rapidjson/error/en.h"
#include "3rdparty/rapidjson/stringbuffer.h"
#include "3rdparty/rapidjson/writer.h"
#include "base/io/json/Json.h"
#include "base/io/json/JsonRequest.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/interfaces/IClientListener.h"
#include "base/kernel/Platform.h"
#include "base/tools/Chrono.h"
#include "base/tools/cryptonote/BlobReader.h"
#include "base/tools/Cvt.h"
#include "net/JobResult.h"

#include <cstring>
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace xmrig {

std::map<int, WebSocketClient*> WebSocketClient::s_clients;
int WebSocketClient::s_nextHandle = 1;

} /* namespace xmrig */

#ifdef __EMSCRIPTEN__

extern "C" {

EM_JS(void, ws_connect, (int handle, const char* url), {
    var urlStr = UTF8ToString(url);
    var ws = new WebSocket(urlStr);
    ws.binaryType = 'arraybuffer';
    Module['__ws_' + handle] = ws;

    ws.onopen = function() {
        dynCall('vi', Module['__ws_onopen'], [handle]);
    };
    ws.onmessage = function(ev) {
        var data = ev.data;
        if (typeof data !== 'string') {
            data = new TextDecoder().decode(new Uint8Array(data));
        }
        var len = lengthBytesUTF8(data) + 1;
        var buf = _malloc(len);
        stringToUTF8(data, buf, len);
        dynCall('viii', Module['__ws_onmessage'], [handle, buf, len - 1]);
        _free(buf);
    };
    ws.onclose = function() {
        dynCall('vi', Module['__ws_onclose'], [handle]);
    };
    ws.onerror = function() {
        dynCall('vi', Module['__ws_onclose'], [handle]);
    };
});

EM_JS(void, ws_send, (int handle, const char* data), {
    var ws = Module['__ws_' + handle];
    if (ws && ws.readyState === 1) {
        ws.send(UTF8ToString(data));
    }
});

EM_JS(void, ws_close, (int handle), {
    var ws = Module['__ws_' + handle];
    if (ws) {
        ws.close();
        delete Module['__ws_' + handle];
    }
});

EM_JS(void, ws_register_callbacks, (int onopen, int onmessage, int onclose), {
    Module['__ws_onopen'] = onopen;
    Module['__ws_onmessage'] = onmessage;
    Module['__ws_onclose'] = onclose;
});

static void onWsOpen(int handle)
{
    auto it = xmrig::WebSocketClient::s_clients.find(handle);
    if (it != xmrig::WebSocketClient::s_clients.end()) {
        it->second->onOpen();
    }
}

static void onWsMessage(int handle, char* data, int len)
{
    auto it = xmrig::WebSocketClient::s_clients.find(handle);
    if (it != xmrig::WebSocketClient::s_clients.end()) {
        it->second->onMessage(data, len);
    }
}

static void onWsClose(int handle)
{
    auto it = xmrig::WebSocketClient::s_clients.find(handle);
    if (it != xmrig::WebSocketClient::s_clients.end()) {
        it->second->onClose();
    }
}

} // extern "C"

#endif

xmrig::WebSocketClient::WebSocketClient(int id, const char *agent, IClientListener *listener) :
    BaseClient(id, listener),
    m_agent(agent)
{
    m_wsHandle = s_nextHandle++;
    s_clients[m_wsHandle] = this;

#ifdef __EMSCRIPTEN__
    static bool registered = false;
    if (!registered) {
        registered = true;
        ws_register_callbacks(
            reinterpret_cast<int>(onWsOpen),
            reinterpret_cast<int>(onWsMessage),
            reinterpret_cast<int>(onWsClose)
        );
    }
#endif
}

xmrig::WebSocketClient::~WebSocketClient()
{
#ifdef __EMSCRIPTEN__
    ws_close(m_wsHandle);
#endif
    s_clients.erase(m_wsHandle);
}

bool xmrig::WebSocketClient::disconnect()
{
    m_keepAlive = 0;
    m_expire    = 0;
    m_failures  = -1;

#ifdef __EMSCRIPTEN__
    ws_close(m_wsHandle);
#endif
    setState(ClosingState);
    return true;
}

bool xmrig::WebSocketClient::isTLS() const
{
    return false;
}

const char *xmrig::WebSocketClient::tlsFingerprint() const
{
    return nullptr;
}

const char *xmrig::WebSocketClient::tlsVersion() const
{
    return nullptr;
}

int64_t xmrig::WebSocketClient::send(const rapidjson::Value &obj, Callback callback)
{
    assert(obj["id"] == sequence());
    m_callbacks.insert({ sequence(), std::move(callback) });
    return send(obj);
}

int64_t xmrig::WebSocketClient::send(const rapidjson::Value &obj)
{
    using namespace rapidjson;
    StringBuffer buffer(0, 512);
    Writer<StringBuffer> writer(buffer);
    obj.Accept(writer);

    const size_t size = buffer.GetSize();
    if (size > 1024 * 16) {
        return -1;
    }

    if (m_sendBuf.size() < size + 2) {
        m_sendBuf.resize(size + 2);
    }

    memcpy(m_sendBuf.data(), buffer.GetString(), size);
    m_sendBuf[size] = '\n';
    m_sendBuf[size + 1] = '\0';

#ifdef __EMSCRIPTEN__
    ws_send(m_wsHandle, m_sendBuf.data());
#endif

    m_expire = Chrono::steadyMSecs() + kResponseTimeout;

    return sequence();
}

int64_t xmrig::WebSocketClient::submit(const JobResult &result)
{
    if (result.jobId != m_job.id()) {
        return -1;
    }

    char nonce[9] = { 0 };
    char data[65] = { 0 };

    Cvt::toHex(nonce, sizeof(nonce), reinterpret_cast<const uint8_t *>(&result.nonce), sizeof(uint32_t));
    Cvt::toHex(data, sizeof(data), result.result(), 32);

    using namespace rapidjson;
    Document doc(kObjectType);
    auto &allocator = doc.GetAllocator();

    Value params(kObjectType);
    params.AddMember("id",     StringRef(m_rpcId.data()), allocator);
    params.AddMember("job_id", StringRef(result.jobId.data()), allocator);
    params.AddMember("nonce",  StringRef(nonce), allocator);
    params.AddMember("result", StringRef(data), allocator);

    JsonRequest::create(doc, sequence(), "submit", params);

    const int64_t id = send(doc);
    if (id >= 0) {
        m_results.insert({ id, SubmitResult(id, result.diff, result.actualDiff(), 0, result.backend) });
    }

    return id;
}

void xmrig::WebSocketClient::connect()
{
    if (m_state == ConnectedState || m_state == ConnectingState) {
        return;
    }

    setState(HostLookupState);

    m_ip     = "127.0.0.1";
    m_rpcId  = "";
    m_failures++;
    m_jobs   = 0;

    setState(ConnectingState);

#ifdef __EMSCRIPTEN__
    std::string urlStr = m_pool.url().data();
    if (urlStr.find("tcp://") == 0) {
        urlStr = "ws://" + urlStr.substr(6);
    }
    else if (urlStr.find("ssl://") == 0) {
        urlStr = "wss://" + urlStr.substr(6);
    }
    ws_connect(m_wsHandle, urlStr.c_str());
#endif

    startTimeout();
}

void xmrig::WebSocketClient::connect(const Pool &pool)
{
    setPool(pool);
    connect();
}

void xmrig::WebSocketClient::deleteLater()
{
    disconnect();
}

void xmrig::WebSocketClient::tick(uint64_t now)
{
    if (m_state != ConnectedState) {
        return;
    }

    if (m_expire && now > m_expire) {
        if (!isQuiet()) {
            LOG_ERR("%s " RED("read error: \"response timeout\""), tag());
        }
        reconnect();
        return;
    }

    if (m_keepAlive && now > m_keepAlive) {
        ping();
    }
}

void xmrig::WebSocketClient::onOpen()
{
    setState(ConnectedState);
    m_failures = 0;
    m_expire   = 0;
    login();
}

void xmrig::WebSocketClient::onMessage(const char *data, size_t len)
{
    std::vector<char> buf(data, data + len);
    buf.push_back('\0');
    parse(buf.data(), buf.size() - 1);
}

void xmrig::WebSocketClient::onClose()
{
    if (m_state == ClosingState) {
        setState(UnconnectedState);
        return;
    }

    if (m_failures == -1) {
        return;
    }

    reconnect();
}

void xmrig::WebSocketClient::login()
{
    m_results.clear();
    m_extensions.reset();

    rapidjson::Document doc;
    auto &allocator = doc.GetAllocator();

    JsonRequest::create(doc, sequence(), "login", doc);

    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("login", m_user.toJSON(), allocator);
    params.AddMember("pass",  m_password.toJSON(), allocator);
    params.AddMember("agent", String(m_agent).toJSON(), allocator);
    params.AddMember("rigid", m_rigId.toJSON(), allocator);

    if (!m_pool.algorithm().isValid()) {
        rapidjson::Value algo(rapidjson::kArrayType);
        algo.PushBack("rx/0", allocator);
        params.AddMember("algo", algo, allocator);
    }

    doc.AddMember("params", params, allocator);

    send(doc, [](const rapidjson::Value &result, bool success, uint64_t) {
        // handled in parseResponse
    });
}

void xmrig::WebSocketClient::parse(char *line, size_t)
{
    using namespace rapidjson;
    Document doc;
    doc.ParseInsitu(line);

    if (doc.HasParseError() || !doc.IsObject()) {
        LOG_ERR("%s " RED("JSON parse error"), tag());
        return;
    }

    const Value &id = doc["id"];
    const Value &result = doc["result"];
    const Value &error  = doc["error"];

    if (id.IsInt64()) {
        parseResponse(id.GetInt64(), result, error);
        return;
    }

    if (id.IsNull()) {
        const Value &method = doc["method"];
        if (method.IsString()) {
            parseNotification(method.GetString(), doc["params"], error);
        }
    }
}

void xmrig::WebSocketClient::parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error)
{
    if (handleResponse(id, result, error)) {
        return;
    }

    if (!result.IsObject()) {
        return;
    }

    int code = -1;
    if (parseLogin(result, &code)) {
        parseExtensions(result);
        m_listener->onLoginSuccess(this);
        return;
    }

    if (!isQuiet()) {
        LOG_ERR("%s " RED("login error code: %d"), tag(), code);
    }

    m_failures++;
    disconnect();
}

bool xmrig::WebSocketClient::parseLogin(const rapidjson::Value &result, int *code)
{
    if (!result.IsObject()) {
        return false;
    }

    const char *rpcId = Json::getString(result, "id");
    if (!rpcId || strlen(rpcId) >= 64) {
        *code = 2;
        return false;
    }

    setRpcId(rpcId);

    const rapidjson::Value &job = result["job"];
    if (job.IsObject()) {
        int jobCode = -1;
        if (!parseJob(job, &jobCode)) {
            *code = jobCode;
            return false;
        }
    }

    const char *s = Json::getString(result, "status");
    if (!s || strcmp(s, "OK") != 0) {
        *code = 1;
        return false;
    }

    return true;
}

bool xmrig::WebSocketClient::parseJob(const rapidjson::Value &params, int *code)
{
    Job job;
    if (!job.setId(params["job_id"].GetString())) {
        *code = 3;
        return false;
    }

    const char *blob = Json::getString(params, "blob");
    if (!blob) {
        *code = 4;
        return false;
    }

    if (!job.setBlob(blob)) {
        *code = 5;
        return false;
    }

    const char *target = Json::getString(params, "target");
    if (!target) {
        *code = 6;
        return false;
    }

    if (!job.setTarget(target)) {
        *code = 7;
        return false;
    }

    job.setAlgorithm(m_pool.algorithm());

    if (m_job == job) {
        return true;
    }

    m_jobs++;

    if (m_job.clientId() == nullptr || strcmp(m_job.clientId(), job.clientId()) != 0) {
        job.setClientId(m_job.clientId());
    }

    m_job = std::move(job);
    m_listener->onJobReceived(this, m_job, params);

    return true;
}

void xmrig::WebSocketClient::parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &)
{
    if (strcmp(method, "job") != 0) {
        return;
    }

    if (!params.IsObject()) {
        return;
    }

    int code = -1;
    if (!parseJob(params, &code) && !isQuiet()) {
        LOG_ERR("%s " RED("invalid job received."), tag());
    }
}

void xmrig::WebSocketClient::parseExtensions(const rapidjson::Value &result)
{
    const rapidjson::Value &extensions = result["extensions"];
    if (!extensions.IsArray()) {
        return;
    }

    for (const rapidjson::Value &ext : extensions.GetArray()) {
        if (!ext.IsString()) {
            continue;
        }

        if (strcmp(ext.GetString(), "algo") == 0) {
            setExtension(EXT_ALGO, true);
        }
        else if (strcmp(ext.GetString(), "connect") == 0) {
            setExtension(EXT_CONNECT, true);
        }
        else if (strcmp(ext.GetString(), "keepalive") == 0) {
            setExtension(EXT_KEEPALIVE, true);
        }
        else if (strcmp(ext.GetString(), "nicehash") == 0) {
            setExtension(EXT_NICEHASH, true);
        }
        else if (strcmp(ext.GetString(), "tls") == 0) {
            setExtension(EXT_TLS, true);
        }
    }
}

void xmrig::WebSocketClient::ping()
{
    m_keepAlive = 0;

    rapidjson::Document doc;
    JsonRequest::create(doc, sequence(), "keepalived", doc);
    send(doc);
}

void xmrig::WebSocketClient::reconnect()
{
    if (m_failures > m_retries) {
        return;
    }

    disconnect();
    setState(ReconnectingState);

    connect();
}

void xmrig::WebSocketClient::setState(SocketState state)
{
    m_state = state;
}

void xmrig::WebSocketClient::startTimeout()
{
    m_expire = Chrono::steadyMSecs() + kConnectTimeout;
}

bool xmrig::WebSocketClient::isCriticalError(const char *message)
{
    if (!message) {
        return false;
    }

    return strstr(message, "Unauthenticated") || strstr(message, "your IP is banned");
}
