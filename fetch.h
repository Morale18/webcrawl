#ifndef FETCH_H
#define FETCH_H

typedef struct buffer{
    char *data;
    size_t len;
}buffer_t;
int fetch_url(void *curl_handle, const char *url, buffer_t *out);
void extract_links(const char *html, const char *base_url,
        void (*cb)(const char *url, void *ctx), void(*ctx));
#endif



