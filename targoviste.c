
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "targoviste.h"

/**
 * Raw header type.
 * This type is part of internal API.
 */

typedef struct {
    char name[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char _padding[255];
} tgx_raw_header_t;

/**
 * Round up n to be divisible by incr.
 * This function is part of internal API
 */

static unsigned round_up(unsigned n, unsigned incr) {
    return n + (incr - n % incr) % incr;
}

/**
 * Calculate checksum of raw header
 * This function is part of internal API.
 */

static unsigned checksum(const tgx_raw_header_t * rh) {
    unsigned i;
    unsigned char * p = (unsigned char *) rh;
    unsigned res = 256;
    for (i = 0; i < offsetof(tgx_raw_header_t, checksum); i++)
        res += p[i];
    for (i = offsetof(tgx_raw_header_t, type); i < sizeof(*rh); i++)
        res += p[i];
    return res;
}


/**
 * Wrappers standarizing user-defined functions
 * These functions are part of internal API.
 */

static int tread(tgx_t * tar, void * data, unsigned size) {
    int err = tar->read(tar, data, size);
    tar->pos += size;
    return err;
}


static int twrite(tgx_t * tar, const void * data, unsigned size) {
    int err = tar->write(tar, data, size);
    tar->pos += size;
    return err;
}

/**
 * Write n null bytes to archive
 * This function is part of internal API.
 */

static int write_null_bytes(tgx_t * tar, int n) {
    int i, err;
    char nul = '\0';
    for (i = 0; i < n; i++) {
        err = twrite(tar, &nul, 1);
        if (err)
            return err;
    }
    return TARGOVISTE_ESUCCES;
}

/**
 * Translate tgx_header_raw_t to tgx_header_t
 * This function is part of internal API.
 */

static int raw_to_header(tgx_header_t * h, const tgx_raw_header_t * rh) {
    unsigned chksum1, chksum2;
    if (*rh->checksum == '\0')
        return TARGOVISTE_ENULLRECORD;
    chksum1 = checksum(rh);
    sscanf(rh->checksum, "%o", &chksum2);
    if (chksum1 != chksum2)
        return TARGOVISTE_EBADCHK;
    sscanf(rh->mode, "%o", &h->mode);
    sscanf(rh->owner, "%o", &h->owner);
    sscanf(rh->size, "%o", &h->size);
    sscanf(rh->mtime, "%o", &h->mtime);
    h->type = rh->type;
    strcpy(h->name, rh->name);
    strcpy(h->linkname, rh->linkname);
    return TARGOVISTE_ESUCCES;
}

/**
 * Translate tgx_header_t to tgx_raw_header_t
 * This function is part of internal API.
 */

static int header_to_raw(tgx_raw_header_t * rh, const tgx_header_t * h) {
    unsigned chksum;
    memset(rh, 0, sizeof(*rh));
    sprintf(rh->mode, "%o", h->mode);
    sprintf(rh->owner, "%o", h->owner);
    sprintf(rh->size, "%o", h->size);
    sprintf(rh->mtime, "%o", h->mtime);
    rh->type = h->type ? h->type : TARGOVISTE_TREG;
    strcpy(rh->name, h->name);
    strcpy(rh->linkname, h->linkname);
    chksum = checksum(rh);
    sprintf(rh->checksum, "%06o", chksum);
    rh->checksum[7] = ' ';
    return TARGOVISTE_ESUCCES;
}

/**
 * Wrappers over file writing & reading, you may override
 * them to integrate your compression method to Targoviste
 * These functions are part of internal API.
 */

static int file_write(tgx_t * tar, const void * data, unsigned size) {
    unsigned res = fwrite(data, 1, size, tar->stream);
    return (res == size) ? TARGOVISTE_ESUCCES : TARGOVISTE_EWRITEFAIL;
}

static int file_read(tgx_t * tar, void * data, unsigned size) {
    unsigned res = fread(data, 1, size, tar->stream);
    return (res == size) ? TARGOVISTE_ESUCCES : TARGOVISTE_EREADFAIL;
}

static int file_seek(tgx_t * tar, unsigned offset) {
    int res = fseek(tar->stream, offset, SEEK_SET);
    return (res == 0) ? TARGOVISTE_ESUCCES : TARGOVISTE_ESEEKFAIL;
}

static int file_close(tgx_t * tar) {
    fclose(tar->stream);
    return TARGOVISTE_ESUCCES;
}



int tgx_open(tgx_t * tar, const char * filename, const char * mode) {
    int err;
    tgx_header_t h;
    memset(tar, 0, sizeof(*tar));
    tar->write = file_write;
    tar->read = file_read;
    tar->seek = file_seek;
    tar->close = file_close;
    if ( strchr(mode, 'r') ) mode = "rb";
    if ( strchr(mode, 'w') ) mode = "wb";
    if ( strchr(mode, 'a') ) mode = "ab";
    tar->stream = fopen(filename, mode);
    if (!tar->stream)
        return TARGOVISTE_EOPENFAIL;
    if (*mode == 'r') {
        err = tgx_read_header(tar, &h);
        if (err != TARGOVISTE_ESUCCES) {
            tgx_close(tar);
            return err;
        }
    }
    return TARGOVISTE_ESUCCES;
}

int tgx_close(tgx_t * tar) {
    return tar->close(tar);
}

int tgx_seek(tgx_t * tar, unsigned pos) {
    int err = tar->seek(tar, pos);
    tar->pos = pos;
    return err;
}

int tgx_rewind(tgx_t * tar) {
    tar->remaining_data = 0;
    tar->last_header = 0;
    return tgx_seek(tar, 0);
}

/**
 * TGX_NEXT() - Seek to next record in archive
 *
 * Parameters:
 * tgx_t * tar - Targoviste archive instance
 *
 * Return value:
 * See TGX_SEEK()
 */

int tgx_next(tgx_t * tar) {
    int err, n;
    tgx_header_t h;
    err = tgx_read_header(tar, &h);
    if (err)
        return err;
    n = round_up(h.size, 512) + sizeof(tgx_raw_header_t);
    return tgx_seek(tar, tar->pos + n);
}

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

int tgx_find(tgx_t * tar, const char * name, tgx_header_t * h) {
    int err;
    tgx_header_t header;
    err = tgx_rewind(tar);
    if (err)
        return err;
    while ( (err = tgx_read_header(tar, &header)) == TARGOVISTE_ESUCCES ) {
        if ( !strcmp(header.name, name) ) {
            if (h)
                *h = header;
            return TARGOVISTE_ESUCCES;
        }
        tgx_next(tar);
    }
    if (err == TARGOVISTE_ENULLRECORD)
        err = TARGOVISTE_ENOTFOUND;
    return err;
}

/**
 * TGX_READ_HEADER() - Read header from archive
 *
 * Parameters:
 * tgx_t * tar      - Targoviste archive instance
 * tgx_header_t * h - Allocated structure to hold read data
 */

int tgx_read_header(tgx_t * tar, tgx_header_t * h) {
    int err;
    tgx_raw_header_t rh;
    tar->last_header = tar->pos;
    err = tread(tar, &rh, sizeof(rh));
    if (err)
        return err;
    err = tgx_seek(tar, tar->last_header);
    if (err)
        return err;
    return raw_to_header(h, &rh);
}

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

int tgx_read_data(tgx_t * tar, void * ptr, unsigned size) {
    int err;
    if (tar->remaining_data == 0) {
        tgx_header_t h;
        err = tgx_read_header(tar, &h);
        if (err)
            return err;
        err = tgx_seek(tar, tar->pos + sizeof(tgx_raw_header_t));
        if (err)
            return err;
        tar->remaining_data = h.size;
    }
    err = tread(tar, ptr, size);
    if (err)
        return err;
    tar->remaining_data -= size;
    if (tar->remaining_data == 0)
        return tgx_seek(tar, tar->last_header);
    return TARGOVISTE_ESUCCES;
}

int tgx_write_header(tgx_t * tar, const tgx_header_t * h) {
    tgx_raw_header_t rh;
    header_to_raw(&rh, h);
    tar->remaining_data = h->size;
    return twrite(tar, &rh, sizeof(rh));
}


int tgx_write_file_header(tgx_t * tar, const char * name, unsigned size) {
    tgx_header_t h;
    memset(&h, 0, sizeof(h));
    strcpy(h.name, name);
    h.size = size;
    h.type = TARGOVISTE_TREG;
    h.mode = 0664;
    return tgx_write_header(tar, &h);
}

int tgx_write_dir_header(tgx_t * tar, const char * name) {
    tgx_header_t h;
    memset(&h, 0, sizeof(h));
    strcpy(h.name, name);
    h.type = TARGOVISTE_TDIR;
    h.mode = 0775;
    return tgx_write_header(tar, &h);
}

int tgx_write_data(tgx_t * tar, const void * data, unsigned size) {
    int err = twrite(tar, data, size);
    if (err)
        return err;
    tar->remaining_data -= size;
    if (tar->remaining_data == 0)
        return write_null_bytes(tar, round_up(tar->pos, 512) - tar->pos);
    return TARGOVISTE_ESUCCES;
}


int tgx_finish(tgx_t * tar) {
    return write_null_bytes(tar, sizeof(tgx_raw_header_t) * 2);
}

void tgx_strerror(char * message, int err) {
    char * error;
    fputs(message, stderr);
    switch (err) {
        case TARGOVISTE_ESUCCES: error = ": no error";
        case TARGOVISTE_EFAILURE: error = ": failure";
        case TARGOVISTE_EOPENFAIL: error = ": could not open";
        case TARGOVISTE_EREADFAIL: error = ": could not read";
        case TARGOVISTE_EWRITEFAIL: error = ": could not write";
        case TARGOVISTE_ESEEKFAIL: error = ": could not seek";
        case TARGOVISTE_EBADCHK: error = ": bad checksum";
        case TARGOVISTE_ENULLRECORD: error = ": null record";
        case TARGOVISTE_ENOTFOUND: error = ": file not found";
        default: error = ": no error";
    }
    fputs(error, stderr);
}
