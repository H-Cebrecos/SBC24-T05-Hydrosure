#include "./wifi_config.c"
#include "esp_err.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

const char *html_config = 
        "<!DOCTYPE html><html><body>"
        "<h2>Configure Wi-Fi</h2>"
        "<form action=\"/submit\" method=\"POST\">"
        "SSID:<br><input type=\"text\" name=\"ssid\"><br><br>"
        "Password:<br><input type=\"password\" name=\"password\"><br><br>"
        "<input type=\"submit\" value=\"Submit\">"
        "</form></body></html>";

void replace_sequence(char *str) {
    const char *target = "%24";
    const char *replacement = "$\0";

    char *pos;
    pos = strstr(str, target);{
        if (pos != NULL){
            strncpy(pos, replacement, 2);
        }
        
    }
}

esp_err_t get_page_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html_config, strlen(html_config));
  return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req) {
  char buff[100];
  int rev_size = req->content_len;

  int ret = httpd_req_recv(req, buff, rev_size);
  if (ret <= 0) {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }
  buff[ret] = 0; // null terminate recieved content.

  printf("%s", buff);
  char *ssid_start = strstr(buff, "ssid=");
  if (ssid_start) {
    sscanf(ssid_start, "ssid=%31[^&]", ssid);
    printf("ssid recieved: %s\n", ssid);
  }

  printf("1\n");
  char *pass_start = strstr(buff, "password=");
  printf("2\n");
  if (ssid_start) {
    sscanf(pass_start, "password=%63s", passw);
    printf("3\n");
    replace_sequence(passw);
    printf("4\n");
    //printf("\n\n\n\n%i\n\n\n\n\n",strcmp(passw, "SBCwifi$"));
    printf("password recieved: %s\n", passw);
  }

  new_config = true;
  const char *resp_str = "config set.";
  httpd_resp_send(req, resp_str, strlen(resp_str));
  return ESP_OK;
}

httpd_handle_t start_config_server() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();

  httpd_handle_t server = NULL;

  httpd_uri_t page_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = get_page_handler,
      .user_ctx = NULL,
  };

  httpd_uri_t button_uri = {
      .uri = "/submit",
      .method = HTTP_POST,
      .handler = post_handler,
      .user_ctx = NULL,
  };

  httpd_start(&server, &cfg);

  httpd_register_uri_handler(server, &page_uri);
  httpd_register_uri_handler(server, &button_uri);
  printf("\nhttp server started.\n");
  return server;
}

void stop_server(httpd_handle_t server) { httpd_stop(server); }
