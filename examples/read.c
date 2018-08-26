
#include <targoviste.h>

int main(void) {
    tgx_t tar;
    const char * str1 = "Hello world";
    /* Open archive */
    tgx_open(&tar, "example.tgx", "w");
    /* Write something */
    tgx_write_file_header(&tar, "example.txt", strlen(str1));
    tgx_write_data(&tar, str1, strlen(str1));
    /* "Finish" archive by writing pair of null bytes */
    tgx_finish(&tar);
    /* Close archive */
    tgx_close(&tar);
}
