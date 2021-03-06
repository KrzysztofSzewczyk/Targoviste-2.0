
/*
MIT License
Copyright (c) 2018 Krzysztof Szewczyk
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TARGOVISTE_H
#define TARGOVISTE_H

#define TARGOVISTE_VERSION "2.0"

/**
 * This enum holds all possible errors that can occur
 * when calling any Targoviste function.
 */

enum {
    TARGOVISTE_ESUCCES      =  0,
    TARGOVISTE_EFAILURE     = -1,
    TARGOVISTE_EOPENFAIL    = -2,
    TARGOVISTE_EREADFAIL    = -3,
    TARGOVISTE_EWRITEFAIL   = -4,
    TARGOVISTE_ESEEKFAIL    = -5,
    TARGOVISTE_EBADCHK      = -6,
    TARGOVISTE_ENULLRECORD  = -7,
    TARGOVISTE_ENOTFOUND    = -8
};

/**
 * Types of files that archive can contain.
 */

enum {
    TARGOVISTE_TREG   = '0',
    TARGOVISTE_TLNK   = '1',
    TARGOVISTE_TSYM   = '2',
    TARGOVISTE_TCHR   = '3',
    TARGOVISTE_TBLK   = '4',
    TARGOVISTE_TDIR   = '5',
    TARGOVISTE_TFIFO  = '6'
};

/**
 * Non-raw header type.
 */

typedef struct {
    unsigned mode;
    unsigned owner;
    unsigned size;
    unsigned mtime;
    unsigned type;
    char name[100];
    char linkname[100];
} tgx_header_t;


/**
 * To omit need to type struct tgx_t everytime.
 */

typedef struct tgx_t tgx_t;

/**
 * Archive structure containing general archive data.
 */

struct tgx_t {
    void * stream;
    unsigned pos;
    unsigned remaining_data;
    unsigned last_header;
};

/**
 * TGX_STRERROR() - same as perror(), but requires error code to be passed.
 */

void tgx_strerror(char * message, int err);

/**
 * TGX_OPEN() - Open filename as archive in selected mode.
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * char * filename - Archive name
 * char * mode - Mode, same as passed to fopen().
 *
 * Return value:
 * ESUCCESS on success.
 */

int tgx_open(tgx_t * tar, const char * filename, const char * mode);

/**
 * TGX_CLOSE() - Close archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 *
 * Return value:
 * Depends on implementation, usually ESUCCESS on success.
 */

int tgx_close(tgx_t * tar);

/**
 * TGX_SEEK() - Seek to specified position in archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * int pos     - position
 *
 * Return value:
 * Depending on implementation, usually ESUCCESS on success
 */

int tgx_seek(tgx_t * tar, unsigned pos);

/**
 * TGX_REWIND() - Rewind archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 *
 * Return value:
 * See TGX_SEEK()
 */

int tgx_rewind(tgx_t * tar);

/**
 * TGX_NEXT() - Seek to next record in archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 *
 * Return value:
 * See TGX_SEEK()
 */

int tgx_next(tgx_t * tar);

/**
 * TGX_FIND() - Find filename in archive, and return it's header
 *
 * Parameters:
 * tgx_t * tar      - Targoviste archive instance
 * char * name      - name of file to find
 * tgx_header_t * h - Pointer to allocated header structure
 *
 * Return value:
 * If file was found, memory under last parameter is now containing header data
 * and return value is equal to ESUCCES
 */

int tgx_find(tgx_t * tar, const char * name, tgx_header_t * h);

/**
 * TGX_READ_HEADER() - Read header from archive
 *
 * Parameters:
 * tgx_t * tar      - Targoviste archive instance
 * tgx_header_t * h - Allocated structure to hold read data
 */

int tgx_read_header(tgx_t * tar, tgx_header_t * h);

/**
 * TGX_READ_DATA() - Read data from archive to buffer
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * void * ptr  - Pointer to buffer
 * int size    - Amount of bytes to read
 *
 * Return value:
 * TARGOVISTE_ESUCCES if operation was completed successfully
 */

int tgx_read_data(tgx_t * tar, void * ptr, unsigned size);

/**
 * TGX_WRITE_HEADER() - Write generic header to archive
 *
 * Parameters:
 * tgx_t * tar      - Targoviste archive instance
 * tgx_header_t * h - header to write
 */

int tgx_write_header(tgx_t * tar, const tgx_header_t * h);

/**
 * TGX_WRITE_FILE_HEADER() - Write file header to archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * char * name - name of file
 * int size    - size of file
 *
 * Return value:
 * See TGX_WRITE_HEADER()
 */

int tgx_write_file_header(tgx_t * tar, const char * name, unsigned size);

/**
 * TGX_WRITE_DIR_HEADER() - Write directory header to archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * char * name - name of directory
 *
 * Return value:
 * See TGX_WRITE_HEADER()
 */

int tgx_write_dir_header(tgx_t * tar, const char * name);

/**
 * TGX_WRITE_DATA() - Write data with specified size to archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 * void * data - data to write
 * int size    - size of data
 *
 * Return value:
 * TARGOVISTE_ESUCCESS if data was written successfully
 */

int tgx_write_data(tgx_t * tar, const void * data, unsigned size);

/**
 * TGX_FINISH() - Save two null bytes to archive marking it's end
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 *
 * Return value:
 * TARGOVISTE_ESUCCESS if bytes were written successfully
 */

int tgx_finish(tgx_t * tar);


#endif
