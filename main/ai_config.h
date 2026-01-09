#pragma once

// AI Assistant Configuration

// Agent Share Code for Authentication
#define XIAOZHI_AGENT_CODE "oWY3UMsM6w"

// XiaoZhi Server URL
// Note: Official server usually uses WSS, but for simplicity/testing we might
// use WS or standard endpoint. Based on xiaozhi-esp32, the default might be
// dynamic, but we'll define the core host here.
#define XIAOZHI_SERVER_URL "ws://api.xiaozhi.me/ws"
// Or specific endpoint if found in docs. For now using a placeholder that looks
// standard. Actually, let's verify if we need a specific token endpoint.
// usually it's ws://[server]:[port]/ws?token=[token]

// WiFi Credentials (User Provided)
#define WIFI_SSID "ASUS"
#define WIFI_PASS "266203A278"
