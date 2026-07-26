#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <curl/curl.h>
#include "fetch.h"

static size_t write_cb(void *ptr, size_t size, size_t nmemb,
        void *userdata){
    buffer_t *buf = userdata;
    size_t total = size * nmemb;
    char *resized = realloc(buf->data, buf->len + total + 1);
    if(!resized){
        return 0;
    }
    buf->data = resized;
    memcpy(buf->data + buf->len, ptr, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}

int fetch_url(void *curl_handle, const char *url, buffer_t *out){
    CURL *curl = curl_handle;
    out->data = NULL;
    out->len = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "simple-crawler/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        free(out->data);
        out->data = NULL;
        return -1;
    }
    return 0;
}



static char *resolve_url(const char *base, const char *href) {
    if (strncasecmp(href, "http://", 7) == 0 || strncasecmp(href, "https://", 8) == 0) {
        return strdup(href);
    }
    if (strncmp(href, "//", 2) == 0) {
        const char *scheme_end = strstr(base, "://");
        size_t scheme_len = scheme_end ? (size_t)(scheme_end - base) : 4;
        char *result = malloc(scheme_len + strlen(href) + 2);
        sprintf(result, "%.*s:%s", (int)scheme_len, base, href);
        return result;
    }
    if (href[0] == '/') {
        const char *p = strstr(base, "://");
        p = p ? p + 3 : base;
        const char *path_start = strchr(p, '/');
        size_t authority_len = path_start ? (size_t)(path_start - base) : strlen(base);
        char *result = malloc(authority_len + strlen(href) + 1);
        sprintf(result, "%.*s%s", (int)authority_len, base, href);
        return result;
    }
    return NULL; 
}


void extract_links(const char *html, const char *base_url,
        void (*cb)(const char *url, void *ctx), void(*ctx)){

    if (!html) return;

    const char *p = html;
    while ((p = strcasestr(p, "href="))){
        p += 5;
        char quote = *p;
        if(quote != '"' && quote != '\'') {p++; continue;}
        p++;
        const char *end = strchr(p, quote);
        if (!end) break;

        size_t len = (size_t)(end - p);
        char *href = malloc(len+1);
        memcpy(href, p, len);
        href[len] = '\0';

        char *resolved = resolve_url(base_url, href);
        if (resolved){
            cb(resolved, ctx);
            free(resolved);
        }
        free(href);
        p = end + 1;
    }

}











