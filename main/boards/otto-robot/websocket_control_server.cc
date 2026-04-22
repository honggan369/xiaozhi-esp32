#include "websocket_control_server.h"
#include "mcp_server.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <sys/param.h>
#include <cstring>
#include <cstdlib>
#include <map>

static const char* TAG = "WSControl";

WebSocketControlServer* WebSocketControlServer::instance_ = nullptr;

static const char kOttoControlPage[] = R"OTTOHTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Otto Robot 控制台</title>
  <style>
    :root {
      --bg: #f7f8fb;
      --card: #ffffff;
      --text: #1f2937;
      --sub: #6b7280;
      --primary: #0f766e;
      --primary-weak: #d1fae5;
      --border: #e5e7eb;
      --danger: #b91c1c;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
      background: radial-gradient(circle at 0 0, #e6fffa, var(--bg) 45%);
      color: var(--text);
    }
    .wrap {
      max-width: 1100px;
      margin: 0 auto;
      padding: 20px 14px 32px;
    }
    .top {
      display: grid;
      grid-template-columns: 1fr auto;
      gap: 12px;
      align-items: center;
      margin-bottom: 14px;
    }
    .title {
      margin: 0;
      font-size: 24px;
    }
    .status {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border: 1px solid var(--border);
      border-radius: 999px;
      background: var(--card);
      font-size: 13px;
      color: var(--sub);
    }
    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #9ca3af;
    }
    .dot.on { background: #059669; }
    .toolbar {
      display: grid;
      grid-template-columns: repeat(6, minmax(90px, 1fr));
      gap: 10px;
      margin-bottom: 14px;
    }
    .toolbar label {
      display: grid;
      gap: 4px;
      font-size: 12px;
      color: var(--sub);
    }
    .toolbar input, .toolbar select, .toolbar button {
      width: 100%;
      border: 1px solid var(--border);
      border-radius: 10px;
      min-height: 36px;
      padding: 8px 10px;
      background: var(--card);
      color: var(--text);
    }
    .toolbar button {
      cursor: pointer;
      font-weight: 600;
      background: #fff1f2;
      border-color: #fecdd3;
      color: var(--danger);
    }
    .hint {
      margin: 0 0 14px;
      color: var(--sub);
      font-size: 13px;
    }
    .groups {
      display: grid;
      gap: 14px;
    }
    .group h2 {
      margin: 0 0 8px;
      font-size: 16px;
    }
    .cards {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
      gap: 10px;
    }
    .card {
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 12px;
      background: var(--card);
      display: grid;
      gap: 8px;
    }
    .card-name {
      font-size: 15px;
      font-weight: 700;
      margin: 0;
    }
    .meta {
      margin: 0;
      color: var(--sub);
      font-size: 12px;
      line-height: 1.4;
    }
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 10px;
      margin-top: 2px;
    }
    .switch {
      position: relative;
      width: 54px;
      height: 30px;
      flex: 0 0 auto;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      inset: 0;
      cursor: pointer;
      border-radius: 999px;
      background: #d1d5db;
      transition: .2s;
    }
    .slider::before {
      content: "";
      position: absolute;
      left: 4px;
      top: 4px;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: #fff;
      transition: .2s;
      box-shadow: 0 1px 3px rgba(0,0,0,.25);
    }
    .switch input:checked + .slider {
      background: var(--primary);
    }
    .switch input:checked + .slider::before {
      transform: translateX(24px);
    }
    .send-label {
      font-size: 12px;
      color: var(--sub);
    }
    .logs {
      margin-top: 14px;
      padding: 10px;
      border-radius: 12px;
      background: #111827;
      color: #e5e7eb;
      min-height: 70px;
      font-family: Consolas, monospace;
      font-size: 12px;
      line-height: 1.5;
      white-space: pre-wrap;
      word-break: break-word;
    }
    @media (max-width: 800px) {
      .toolbar { grid-template-columns: repeat(2, minmax(120px, 1fr)); }
      .top { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="top">
      <h1 class="title">Otto Robot 动作控制台</h1>
      <div class="status"><span id="dot" class="dot"></span><span id="statusText">未连接</span></div>
    </div>

    <div class="toolbar">
      <label>Steps
        <input id="steps" type="number" min="1" max="100" value="3" />
      </label>
      <label>Speed(ms)
        <input id="speed" type="number" min="100" max="3000" value="700" />
      </label>
      <label>Direction
        <select id="direction">
          <option value="1">1</option>
          <option value="-1">-1</option>
          <option value="0">0</option>
        </select>
      </label>
      <label>Amount
        <input id="amount" type="number" min="0" max="170" value="30" />
      </label>
      <label>Arm Swing
        <input id="arm_swing" type="number" min="0" max="170" value="50" />
      </label>
      <label>&nbsp;
        <button id="stopBtn" type="button">紧急停止并复位</button>
      </label>
    </div>

    <p class="hint">每个动作右侧开关打开后会立即发送一次指令，随后自动回弹。需要手臂的动作在无手臂版本上会被设备拒绝。</p>

    <div id="groups" class="groups"></div>
    <div id="logs" class="logs">[init] 等待建立 WebSocket 连接...</div>
  </div>

  <script>
    const ACTION_GROUPS = [
      {
        title: "基础动作",
        items: [
          { action: "walk", label: "行走", args: ["steps", "speed", "direction", "arm_swing"] },
          { action: "turn", label: "转身", args: ["steps", "speed", "direction", "arm_swing"] },
          { action: "jump", label: "跳跃", args: ["steps", "speed"] },
          { action: "swing", label: "摇摆", args: ["steps", "speed", "amount"] },
          { action: "moonwalk", label: "太空步", args: ["steps", "speed", "direction", "amount"] },
          { action: "bend", label: "弯腰", args: ["steps", "speed", "direction"] },
          { action: "shake_leg", label: "摇腿", args: ["steps", "speed", "direction"] },
          { action: "updown", label: "上下运动", args: ["steps", "speed", "amount"] },
          { action: "whirlwind_leg", label: "旋风腿", args: ["steps", "speed", "amount"] },
          { action: "sit", label: "坐下", args: [] },
          { action: "showcase", label: "展示动作", args: [] },
          { action: "victory_dance", label: "胜利舞", args: ["steps", "speed"] },
          { action: "home", label: "复位", args: [] }
        ]
      },
      {
        title: "手部动作",
        items: [
          { action: "hands_up", label: "举手", args: ["speed", "direction"] },
          { action: "hands_down", label: "放手", args: ["speed", "direction"] },
          { action: "hand_wave", label: "挥手", args: ["direction"] },
          { action: "windmill", label: "大风车", args: ["steps", "speed", "amount"] },
          { action: "takeoff", label: "起飞", args: ["steps", "speed", "amount"] },
          { action: "fitness", label: "健身", args: ["steps", "speed", "amount"] },
          { action: "greeting", label: "打招呼", args: ["steps", "direction"] },
          { action: "shy", label: "害羞", args: ["steps", "direction"] },
          { action: "radio_calisthenics", label: "广播体操", args: [] },
          { action: "magic_circle", label: "魔力转圈", args: [] }
        ]
      }
    ];

    const logsEl = document.getElementById("logs");
    const dotEl = document.getElementById("dot");
    const statusTextEl = document.getElementById("statusText");
    let ws = null;
    let rpcId = 1;

    function log(msg) {
      const time = new Date().toLocaleTimeString();
      logsEl.textContent += "\\n[" + time + "] " + msg;
      logsEl.scrollTop = logsEl.scrollHeight;
    }

    function setConnected(connected) {
      dotEl.classList.toggle("on", connected);
      statusTextEl.textContent = connected ? "已连接" : "未连接";
    }

    function getFieldValue(name) {
      return Number(document.getElementById(name).value);
    }

    function buildArgs(actionDef) {
      const args = { action: actionDef.action };
      for (const key of actionDef.args) {
        args[key] = getFieldValue(key);
      }
      return args;
    }

    function sendMcpToolCall(name, args) {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        log("发送失败：WebSocket 未连接");
        return;
      }
      const payload = {
        jsonrpc: "2.0",
        id: rpcId++,
        method: "tools/call",
        params: { name, arguments: args || {} }
      };
      ws.send(JSON.stringify({ type: "mcp", payload }));
      log("已发送: " + name + " -> " + JSON.stringify(args || {}));
    }

    function connectWs() {
      const protocol = location.protocol === "https:" ? "wss://" : "ws://";
      const url = protocol + location.host + "/ws";
      ws = new WebSocket(url);

      ws.onopen = () => {
        setConnected(true);
        log("WebSocket 连接成功: " + url);
      };

      ws.onmessage = (event) => {
        log("收到设备消息: " + event.data);
      };

      ws.onclose = () => {
        setConnected(false);
        log("连接关闭，2秒后重连...");
        setTimeout(connectWs, 2000);
      };

      ws.onerror = () => {
        log("WebSocket 错误");
      };
    }

    function renderCards() {
      const groupsEl = document.getElementById("groups");
      groupsEl.innerHTML = "";
      for (const group of ACTION_GROUPS) {
        const section = document.createElement("section");
        section.className = "group";
        section.innerHTML = "<h2>" + group.title + "</h2><div class=\"cards\"></div>";
        const cardsEl = section.querySelector(".cards");

        for (const item of group.items) {
          const card = document.createElement("article");
          card.className = "card";
          card.innerHTML = `
            <p class="card-name">${item.label}</p>
            <p class="meta">action: ${item.action}<br>args: ${item.args.length ? item.args.join(", ") : "无"}</p>
            <div class="switch-row">
              <span class="send-label">执行一次</span>
              <label class="switch">
                <input type="checkbox" />
                <span class="slider"></span>
              </label>
            </div>
          `;
          const checkbox = card.querySelector("input[type=checkbox]");
          checkbox.addEventListener("change", () => {
            if (checkbox.checked) {
              sendMcpToolCall("self.otto.action", buildArgs(item));
              setTimeout(() => { checkbox.checked = false; }, 260);
            }
          });
          cardsEl.appendChild(card);
        }
        groupsEl.appendChild(section);
      }
    }

    document.getElementById("stopBtn").addEventListener("click", () => {
      sendMcpToolCall("self.otto.stop", {});
    });

    renderCards();
    connectWs();
  </script>
</body>
</html>
)OTTOHTML";

WebSocketControlServer::WebSocketControlServer() : server_handle_(nullptr) {
    instance_ = this;
}

WebSocketControlServer::~WebSocketControlServer() {
    Stop();
    instance_ = nullptr;
}

esp_err_t WebSocketControlServer::root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, kOttoControlPage, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebSocketControlServer::ws_handler(httpd_req_t *req) {
    if (instance_ == nullptr) {
        return ESP_FAIL;
    }
    
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        instance_->AddClient(req);
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
    
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "WebSocket close frame received");
        instance_->RemoveClient(req);
        free(buf);
        return ESP_OK;
    }
    
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (ws_pkt.len > 0 && buf != nullptr) {
            buf[ws_pkt.len] = '\0';
            instance_->HandleMessage(req, (const char*)buf, ws_pkt.len);
        }
    } else {
        ESP_LOGW(TAG, "Unsupported frame type: %d", ws_pkt.type);
    }
    
    free(buf);
    return ESP_OK;
}

bool WebSocketControlServer::Start(int port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_open_sockets = 7;
    config.ctrl_port = 32769;

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = nullptr
    };

    httpd_uri_t index_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = nullptr
    };

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = nullptr,
        .is_websocket = true
    };

    if (httpd_start(&server_handle_, &config) == ESP_OK) {
        httpd_register_uri_handler(server_handle_, &root_uri);
        httpd_register_uri_handler(server_handle_, &index_uri);
        httpd_register_uri_handler(server_handle_, &ws_uri);
        ESP_LOGI(TAG, "WebSocket server started on port %d", port);
        return true;
    }

    ESP_LOGE(TAG, "Failed to start WebSocket server");
    return false;
}

void WebSocketControlServer::Stop() {
    if (server_handle_) {
        httpd_stop(server_handle_);
        server_handle_ = nullptr;
        clients_.clear();
        ESP_LOGI(TAG, "WebSocket server stopped");
    }
}

void WebSocketControlServer::HandleMessage(httpd_req_t *req, const char* data, size_t len) {
    if (data == nullptr || len == 0) {
        ESP_LOGE(TAG, "Invalid message: data is null or len is 0");
        return;
    }
    
    if (len > 4096) {
        ESP_LOGE(TAG, "Message too long: %zu bytes", len);
        return;
    }
    
    char* temp_buf = (char*)malloc(len + 1);
    if (temp_buf == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return;
    }
    memcpy(temp_buf, data, len);
    temp_buf[len] = '\0';
    
    cJSON* root = cJSON_Parse(temp_buf);
    free(temp_buf);
    
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    // 支持两种格式：
    // 1. 完整格式：{"type":"mcp","payload":{...}}
    // 2. 简化格式：直接是MCP payload对象
    
    cJSON* payload = nullptr;
    cJSON* type = cJSON_GetObjectItem(root, "type");
    
    if (type && cJSON_IsString(type) && strcmp(type->valuestring, "mcp") == 0) {
        payload = cJSON_GetObjectItem(root, "payload");
        if (payload != nullptr) {
            cJSON_DetachItemViaPointer(root, payload);
            McpServer::GetInstance().ParseMessage(payload);
            cJSON_Delete(payload); 
        }
    } else {
        payload = cJSON_Duplicate(root, 1);
        if (payload != nullptr) {
            McpServer::GetInstance().ParseMessage(payload);
            cJSON_Delete(payload);
        }
    }
    
    if (payload == nullptr) {
        ESP_LOGE(TAG, "Invalid message format or failed to parse");
    }

    cJSON_Delete(root);
}

void WebSocketControlServer::AddClient(httpd_req_t *req) {
    int sock_fd = httpd_req_to_sockfd(req);
    if (clients_.find(sock_fd) == clients_.end()) {
        clients_[sock_fd] = req;
        ESP_LOGI(TAG, "Client connected: %d (total: %zu)", sock_fd, clients_.size());
    }
}

void WebSocketControlServer::RemoveClient(httpd_req_t *req) {
    int sock_fd = httpd_req_to_sockfd(req);
    clients_.erase(sock_fd);
    ESP_LOGI(TAG, "Client disconnected: %d (total: %zu)", sock_fd, clients_.size());
}

size_t WebSocketControlServer::GetClientCount() const {
    return clients_.size();
}
