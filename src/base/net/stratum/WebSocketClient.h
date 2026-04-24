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

#ifndef XMRIG_WEBSOCKET_CLIENT_H
#define XMRIG_WEBSOCKET_CLIENT_H

#include "base/net/stratum/BaseClient.h"
#include "base/net/stratum/Job.h"
#include "base/net/stratum/Pool.h"
#include "base/net/stratum/SubmitResult.h"
#include "base/tools/Object.h"

#include <bitset>
#include <map>
#include <string>
#include <vector>

namespace xmrig {

class IClientListener;
class JobResult;

class WebSocketClient : public BaseClient
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebSocketClient)

    constexpr static uint64_t kConnectTimeout   = 20 * 1000;
    constexpr static uint64_t kResponseTimeout  = 20 * 1000;

    WebSocketClient(int id, const char *agent, IClientListener *listener);
    ~WebSocketClient() override;

    void onClose();
    void onMessage(const char *data, size_t len);
    void onOpen();

    static std::map<int, WebSocketClient*> s_clients;

protected:
    bool disconnect() override;
    bool isTLS() const override;
    const char *tlsFingerprint() const override;
    const char *tlsVersion() const override;
    int64_t send(const rapidjson::Value &obj, Callback callback) override;
    int64_t send(const rapidjson::Value &obj) override;
    int64_t submit(const JobResult &result) override;
    void connect() override;
    void connect(const Pool &pool) override;
    void deleteLater() override;
    void tick(uint64_t now) override;

    inline bool hasExtension(Extension extension) const noexcept override   { return m_extensions.test(extension); }
    inline const char *mode() const override                                { return "pool"; }

    inline const char *agent() const                                        { return m_agent; }
    inline const char *url() const                                          { return m_pool.url(); }
    inline const String &rpcId() const                                      { return m_rpcId; }
    inline void setRpcId(const char *id)                                    { m_rpcId = id; }
    inline void setPoolUrl(const char *url)                                 { m_pool.setUrl(url); }

private:
    bool parseJob(const rapidjson::Value &params, int *code);
    bool parseLogin(const rapidjson::Value &result, int *code);
    void login();
    void parse(char *line, size_t len);
    void parseExtensions(const rapidjson::Value &result);
    void parseNotification(const char* method, const rapidjson::Value& params, const rapidjson::Value& error);
    void parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error);
    void ping();
    void reconnect();
    void setState(SocketState state);
    void startTimeout();

    inline void setExtension(Extension ext, bool enable) noexcept   { m_extensions.set(ext, enable); }

    static bool isCriticalError(const char *message);

    const char *m_agent;
    std::bitset<EXT_MAX> m_extensions;
    std::vector<char> m_sendBuf;
    String m_rpcId;
    uint64_t m_expire           = 0;
    uint64_t m_jobs             = 0;
    uint64_t m_keepAlive        = 0;
    int m_wsHandle              = -1;

    static int s_nextHandle;
};

} /* namespace xmrig */

#endif /* XMRIG_WEBSOCKET_CLIENT_H */
