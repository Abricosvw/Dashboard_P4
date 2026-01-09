#include "web_server.h"
#include "dirent.h"
#include "ecu_data.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "include/can_websocket.h"
#include "sd_card_manager.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t s_server = NULL;

#define SCRATCH_BUFSIZE (8192)

/* Simple URL decoder - Moved to top to be available for files_get_handler */
static void url_decode(char *dst, const char *src) {
  char a, b;
  while (*src) {
    if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
        (isxdigit((int)a) && isxdigit((int)b))) {
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else if (*src == '+') {
      *dst++ = ' ';
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

/* Handler to list files */
static esp_err_t files_get_handler(httpd_req_t *req) {
  char buf[512];
  char path[256] = "";

  // Try to get 'path' parameter
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char path_enc[256];
    if (httpd_query_key_value(buf, "path", path_enc, sizeof(path_enc)) ==
        ESP_OK) {
      url_decode(path, path_enc);
    }
  }

  char full_path[512];
  // Ensure path starts with / if not empty, but SD_MOUNT_POINT usually lacks
  // trailing slash
  if (strlen(path) > 0) {
    if (path[0] == '/') {
      snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    } else {
      snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT, path);
    }
  } else {
    snprintf(full_path, sizeof(full_path), "%s", SD_MOUNT_POINT);
  }

  DIR *dir = opendir(full_path);
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open directory %s", full_path);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to open directory");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr_chunk(req, "[");

  struct dirent *ent;
  bool first = true;
  while ((ent = readdir(dir)) != NULL) {
    // skip . and ..
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    if (!first) {
      httpd_resp_sendstr_chunk(req, ",");
    }

    char entry[512];
    if (ent->d_type == DT_DIR) {
      snprintf(entry, sizeof(entry), "{\"name\":\"%s\", \"type\":\"dir\"}",
               ent->d_name);
    } else {
      snprintf(entry, sizeof(entry), "{\"name\":\"%s\", \"type\":\"file\"}",
               ent->d_name);
    }
    httpd_resp_sendstr_chunk(req, entry);
    first = false;
  }
  closedir(dir);

  httpd_resp_sendstr_chunk(req, "]");
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

/* Handler to download a file */
static esp_err_t download_get_handler(httpd_req_t *req) {
  char buf[512];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename");
    return ESP_FAIL;
  }

  char filename_enc[256];
  if (httpd_query_key_value(buf, "file", filename_enc, sizeof(filename_enc)) !=
      ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing file parameter");
    return ESP_FAIL;
  }

  char filename[256];
  url_decode(filename, filename_enc);

  char full_path[512];
  snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT, filename);

  FILE *f = fopen(full_path, "r");
  if (!f) {
    ESP_LOGE(TAG, "File not found: %s", full_path);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "application/octet-stream");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "attachment"); // Force download

  char *chunk = malloc(SCRATCH_BUFSIZE);
  if (!chunk) {
    fclose(f);
    return ESP_FAIL;
  }

  size_t read_bytes;
  do {
    read_bytes = fread(chunk, 1, SCRATCH_BUFSIZE, f);
    if (read_bytes > 0) {
      httpd_resp_send_chunk(req, chunk, read_bytes);
    }
  } while (read_bytes > 0);

  fclose(f);
  free(chunk);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/* Handler to upload a file */
static esp_err_t upload_post_handler(httpd_req_t *req) {
  char buf[512];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename");
    return ESP_FAIL;
  }

  char filename_enc[256];
  if (httpd_query_key_value(buf, "file", filename_enc, sizeof(filename_enc)) !=
      ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing file parameter");
    return ESP_FAIL;
  }

  char filename[256];
  url_decode(filename, filename_enc);

  char full_path[512];
  snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT, filename);

  FILE *f = fopen(full_path, "w");
  if (!f) {
    ESP_LOGE(TAG, "Failed to create file: %s. Error: %s", full_path,
             strerror(errno));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to create file");
    return ESP_FAIL;
  }

  char *chunk = malloc(SCRATCH_BUFSIZE);
  if (!chunk) {
    fclose(f);
    return ESP_FAIL;
  }

  int received;
  int remaining = req->content_len;
  while (remaining > 0) {
    if ((received = httpd_req_recv(
             req, chunk,
             (remaining < SCRATCH_BUFSIZE) ? remaining : SCRATCH_BUFSIZE)) <=
        0) {
      if (received == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      fclose(f);
      free(chunk);
      return ESP_FAIL;
    }
    fwrite(chunk, 1, received, f);
    remaining -= received;
  }
  fclose(f);
  free(chunk);
  httpd_resp_sendstr(req, "File uploaded successfully");
  return ESP_OK;
}

/* Handler to delete a file */
static esp_err_t delete_post_handler(httpd_req_t *req) {
  char buf[512];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename");
    return ESP_FAIL;
  }

  char filename_enc[256];
  if (httpd_query_key_value(buf, "file", filename_enc, sizeof(filename_enc)) !=
      ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing file parameter");
    return ESP_FAIL;
  }

  char filename[256];
  url_decode(filename, filename_enc);

  char full_path[512];
  snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT, filename);

  if (unlink(full_path) == 0) {
    ESP_LOGI(TAG, "Deleted file: %s", full_path);
    httpd_resp_sendstr(req, "File deleted successfully");
  } else {
    ESP_LOGE(TAG, "Failed to delete file: %s", full_path);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Delete failed");
  }

  return ESP_OK;
}

/* Simple index page */
static esp_err_t index_get_handler(httpd_req_t *req) {
  const char *resp =
      "<html><head><title>Dashboard SD Manager</title>"
      "<meta charset='utf-8'>"
      "<style>body{font-family:sans-serif; background:#111; color:#eee; "
      "padding:20px;}"
      "h1{color:#00e5ff;} .file-item{background:#222; padding:10px; margin:5px "
      "0; border-radius:5px; display:flex; justify-content:space-between; "
      "align-items:center;}"
      ".file-item.dir { border-left: 4px solid #00e5ff; }"
      "button, a.btn{background:#444; border:none; color:white; padding:5px "
      "12px; cursor:pointer; border-radius:3px; margin-left:5px; "
      "text-decoration:none; font-size:14px; display:inline-block;}"
      "button.del{background:#ff5252;} button.nav{background:#00e5ff; "
      "color:black;} a.btn:hover{background:#666;} "
      ".controls{margin-bottom:20px; background:#222; padding:15px; "
      "border-radius:5px; display:flex; gap:10px; align-items:center;}"
      "#path-display { font-family:monospace; color:#aaa; margin-left:10px; "
      "flex-grow:1; }</style></head>"
      "<body><h1>SD Card Manager</h1>"
      "<div class='controls'>"
      "  <a href='/joystick' class='btn btn-blue'>🎮 Joystick</a>"
      "  <a href='/dashboard' class='btn'>📊 Dashboard</a>"
      "  <button onclick='navUp()'>⬆ Up</button>"
      "  <span id='path-display'>/</span>"
      "  <input type='file' id='file-input'>"
      "  <button onclick='uploadFile()'>Upload</button>"
      "</div>"
      "<div id='file-list'>Loading...</div>"
      "<script>"
      "let currentPath = '';"
      "async function loadFiles(path = '') {"
      "  currentPath = path;"
      "  document.getElementById('path-display').innerText = (path || '/');"
      "  try { "
      "    const encPath = encodeURIComponent(path);"
      "    const res = await fetch('/api/files?path=' + encPath);"
      "    const files = await res.json();"
      "    const list = document.getElementById('file-list');"
      "    list.innerHTML = files.map(f => {"
      "       const fullPath = path ? (path + '/' + f.name) : f.name;"
      "       const encFull = encodeURIComponent(fullPath);"
      "       if (f.type === 'dir') {"
      "         return `<div class='file-item dir'><span>📁 "
      "${f.name}</span><div>"
      "         <button class='nav' "
      "onclick='loadFiles(\"${fullPath}\")'>Open</button>"
      "         </div></div>`;"
      "       } else {"
      "         return `<div class='file-item'><span>📄 ${f.name}</span><div>"
      "         <a class='btn' href='/api/download?file=${encFull}' "
      "download>Download</a>"
      "         <button class='del' "
      "onclick='deleteFile(\"${encFull}\")'>Delete</button></div></div>`;"
      "       }"
      "    }).join('');"
      "  } catch(e) { console.error(e); }"
      "}"
      "function navUp() {"
      "  if(!currentPath) return;"
      "  const parts = currentPath.split('/');"
      "  parts.pop();"
      "  loadFiles(parts.join('/'));"
      "}"
      "async function deleteFile(encName) {"
      "  if(confirm('Delete file?')) {"
      "    await fetch('/api/delete?file=' + encName, {method: 'POST'});"
      "    loadFiles(currentPath);"
      "  }"
      "}"
      "async function uploadFile() {"
      "  const input = document.getElementById('file-input');"
      "  if(input.files.length === 0) return alert('Select file');"
      "  const file = input.files[0];"
      "  const fullPath = currentPath ? (currentPath + '/' + file.name) : "
      "file.name;"
      "  const enc = encodeURIComponent(fullPath);"
      "  await fetch('/api/upload?file=' + enc, {method: 'POST', body: file});"
      "  alert('Uploaded'); loadFiles(currentPath); input.value = '';"
      "}"
      "loadFiles();"
      "</script></body></html>";
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

/* Handler for Joystick page */
static esp_err_t joystick_get_handler(httpd_req_t *req) {
  extern const uint8_t joystick_html_start[] asm("_binary_joystick_html_start");
  extern const uint8_t joystick_html_end[] asm("_binary_joystick_html_end");
  const size_t joystick_html_size = (joystick_html_end - joystick_html_start);
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, (const char *)joystick_html_start, joystick_html_size);
  return ESP_OK;
}

/* Handler for ECU Data API */
static esp_err_t ecu_data_api_handler(httpd_req_t *req) {
  ecu_data_t data;
  ecu_data_get_copy(&data);

  char json[512];
  snprintf(
      json, sizeof(json),
      "{\"rpm\":%.0f,\"speed\":%.1f,\"clt\":%.1f,\"iat\":%.1f,\"oil\":%.1f,"
      "\"volt\":%.2f,\"gear\":%d,\"boost\":%.1f,\"tps\":%.1f}",
      data.engine_rpm, data.vehicle_speed, data.clt_temp, data.iat_temp,
      data.oil_temp, data.battery_voltage, data.gear, data.map_kpa,
      data.tps_position);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

/* Handler for Dashboard redirect (Temporary until web dashboard is built) */
static esp_err_t dashboard_get_handler(httpd_req_t *req) {
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

esp_err_t web_server_start(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.max_uri_handlers = 12; // Increased for extra handlers

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&s_server, &config) == ESP_OK) {
    httpd_uri_t index_uri = {
        .uri = "/", .method = HTTP_GET, .handler = index_get_handler};
    httpd_register_uri_handler(s_server, &index_uri);

    httpd_uri_t files_uri = {
        .uri = "/api/files", .method = HTTP_GET, .handler = files_get_handler};
    httpd_register_uri_handler(s_server, &files_uri);

    httpd_uri_t download_uri = {.uri = "/api/download",
                                .method = HTTP_GET,
                                .handler = download_get_handler};
    httpd_register_uri_handler(s_server, &download_uri);

    httpd_uri_t upload_uri = {.uri = "/api/upload",
                              .method = HTTP_POST,
                              .handler = upload_post_handler};
    httpd_register_uri_handler(s_server, &upload_uri);

    httpd_uri_t delete_uri = {.uri = "/api/delete",
                              .method = HTTP_POST,
                              .handler = delete_post_handler};
    httpd_register_uri_handler(s_server, &delete_uri);

    httpd_uri_t joystick_uri = {.uri = "/joystick",
                                .method = HTTP_GET,
                                .handler = joystick_get_handler};
    httpd_register_uri_handler(s_server, &joystick_uri);

    httpd_uri_t dashboard_uri = {.uri = "/dashboard",
                                 .method = HTTP_GET,
                                 .handler = dashboard_get_handler};
    httpd_register_uri_handler(s_server, &dashboard_uri);

    httpd_uri_t data_uri = {.uri = "/api/ecu-data",
                            .method = HTTP_GET,
                            .handler = ecu_data_api_handler};
    httpd_register_uri_handler(s_server, &data_uri);

    return ESP_OK;
  }
  return ESP_FAIL;
}

esp_err_t web_server_stop(void) {
  if (s_server) {
    httpd_stop(s_server);
    s_server = NULL;
  }
  return ESP_OK;
}
