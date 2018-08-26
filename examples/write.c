
#include <targoviste.h>

int main() {
    tgx_t tar;
    tgx_header_t h;
    char * p;
    /* Open archive */
    tgx_open(&tar, "example.tgx", "r");
    /* List files */
    while ( (tgx_read_header(&tar, &h)) != TGX_ENULLRECORD ) {
        printf("%s (%d bytes)\n", h.name, h.size);
        tgx_next(&tar);
    }
    /* Read file by name */
    tgx_find(&tar, "example.txt", &h);
    p = calloc(1, h.size + 1);
    tgx_read_data(&tar, p, h.size);
    /* Display file contents */
    printf("%s", p);
    free(p);
    tgx_close(&tar);
}
