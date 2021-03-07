/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Wanding.Zhou@pennmedicine.upenn.edu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#ifndef _TBMATE_H
#define _TBMATE_H

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <inttypes.h>
#include <wordexp.h>
#include "wzmisc.h"


#define PACKAGE_VERSION "1.7.20210306"

/* size of the header element in bytes */
#define HDR_ID         3        /* three letter "tbk" */
#define HDR_VERSION    4
#define HDR_DATA_TYPE  8
#define HDR_NMAX       8
#define HDR_EXTRA      8169      /* message */
#define HDR_TOTALBYTES 8192

#define HDR_NMAX0   (3+4+8) /* offset to max_offset */
#define MAX_DOUBLE16 ((1<<15)-2)

#define DT_INT1          1
#define DT_INT2          2
#define DT_INT32         3
#define DT_FLOAT         4
#define DT_DOUBLE        5
#define DT_STRINGD       6
#define DT_STRINGF       7
#define DT_ONES          30
#define DT_FLOAT_INT     31
#define DT_FLOAT_FLOAT   32
#define DT_NA            99

#define DATA_TYPE(d) ((d)&0xff)
#define STRING_MAX(d) ((d)>>8)

/* typedef enum { */
/*   DT_NA, DT_INT1, DT_INT2, DT_INT32, DT_FLOAT, */
/*   DT_DOUBLE, DT_ONES, DT_STRINGD, DT_STRINGF} data_type_t; */

static inline float uint16_to_float(uint16_t i) {
  return ((float)i - MAX_DOUBLE16) / MAX_DOUBLE16;
}

static inline uint16_t float_to_uint16(float f) {
  return (uint16_t) roundf((f+1) * MAX_DOUBLE16);
}

extern const int unit_base[40];
static inline int unit_size(uint64_t d) {
  int nu = unit_base[DATA_TYPE(d)];
  if (d == DT_STRINGF) {
    nu += STRING_MAX(d);
  }
  return nu;
}

typedef struct tbf_t {
  FILE *fh;
  int64_t offset;               /* where the file has been read */
  char *fname;
  char *sname_first;
} tbf_t;

typedef struct tbk_t {
  char *sname;
  tbf_t *tbf;
  int32_t version;
  /* offset in the number of units or byte if unit is sub-byte */
  int64_t nmax;
  int64_t offset_sample_beg;
  char extra[HDR_EXTRA];
  uint64_t dtype;               /* data type */
  uint8_t data;                 /* sub-byte data */
  int num_samples;
} tbk_t;

static inline void tbf_open1(char *fname, tbf_t *tbf, char *sname) {
  memset(tbf, 0, sizeof(tbf_t));
  tbf->offset = 0;
  tbf->fname = strdup(fname);
  tbf->fh = fopen(fname, "rb");
  if (!tbf->fh) wzfatal("Cannot open %s to read.\n", tbf->fname);
  if (sname != NULL) {tbf->sname_first = strdup(sname);}
}

static inline tbf_t *tbf_open(char *fname) {
  tbf_t *tbf = calloc(1, sizeof(tbf_t));
  tbf_open1(fname, tbf, NULL);
  return tbf;
}

static inline tbf_t *tbf_open_update(char *fname) {
  tbf_t *tbf = calloc(1, sizeof(tbf_t));
  tbf->offset = 0;
  tbf->fname = strdup(fname);
  tbf->fh = fopen(fname, "r+b");
  if (!tbf->fh) wzfatal("Cannot open %s to update.\n", tbf->fname);
  return tbf;
}

static inline void tbf_skip_data(tbk_t *tbk) {
  fseek(tbk->tbf->fh, tbk->nmax * unit_size(tbk->dtype), SEEK_CUR);
  tbk->tbf->offset += tbk->nmax * unit_size(tbk->dtype);
}

static inline void tbf_read(tbf_t *tbf, void *ptr, size_t nbytes, size_t n) {
  fread(ptr, nbytes, n, tbf->fh);
  tbf->offset += nbytes * n;
}

static inline void tbk_seek_n(tbk_t *tbk, int64_t n) {
  int64_t offset = tbk->offset_sample_beg + HDR_TOTALBYTES;
  offset += n * unit_size(tbk->dtype);

  tbf_t *tbf = tbk->tbf;
  if (offset == tbf->offset) return;
  
  tbf->offset = offset;
  if (fseek(tbf->fh, tbf->offset, SEEK_SET))
    wzfatal("File %s cannot be seeked.\n", tbk->tbf->fname);
}

static inline void tbk_seek_offset(tbk_t *tbk, int64_t offset) {

  offset += tbk->offset_sample_beg + HDR_TOTALBYTES;
  
  tbf_t *tbf = tbk->tbf;
  if (offset == tbf->offset) return;
  
  tbf->offset = offset;
  if (fseek(tbf->fh, tbf->offset, SEEK_SET))
    wzfatal("File %s cannot be seeked.\n", tbk->tbf->fname);
}

static inline void tbk_set_sname_by_fname(tbk_t *tbk) {
  char *tmp = strdup(tbk->tbf->fname);
  char *bname = basename(tmp);
  int k = strlen(bname);
  if (k>4 && bname[k-4]=='.' && bname[k-3]=='t' &&
      bname[k-2]=='b' && bname[k-1]=='k')
    bname[k-4] = '\0';
  tbk->sname = strdup(bname);
  free(tmp);
}

static inline void tbk_set_sname_by_extra(tbk_t *tbk) {
  if (tbk->sname == NULL) {
    if (tbk->extra[strlen(tbk->extra) + 2] != '\0') {
      tbk->sname = strdup(&tbk->extra[strlen(tbk->extra) + 2]);
    }
  }
}

static inline void tbf_next(tbf_t *tbf, tbk_t *tbk) {
  memset(tbk, 0, sizeof(tbk_t));
  tbk->offset_sample_beg = tbf->offset;
  tbk->tbf = tbf;
  
  char id[3];
  tbf_read(tbf, id, HDR_ID, 1);

  if (id[0] != 't' || id[1] != 'b' || id[2] != 'k')
    wzfatal("%s is not a valid tbk file.\n", tbf->fname);

  tbf_read(tbf, &tbk->version, HDR_VERSION, 1);
  tbf_read(tbf, &tbk->dtype,   HDR_DATA_TYPE, 1);
  tbf_read(tbf, &tbk->nmax,    HDR_NMAX, 1);
  tbf_read(tbf, &tbk->extra,   HDR_EXTRA, 1);
  tbk_set_sname_by_extra(tbk);
}

void parse_tbk_from_tbf(tbf_t *tbf, tbk_t **tbks, int *n_tbks);

static inline void tbf_close(tbf_t *tbf) {
  fclose(tbf->fh);
  if (tbf->sname_first) free(tbf->sname_first);
  free(tbf->fname);
}

static inline int tbf_num_samples(char *fname) {
  tbf_t *tbf = tbf_open(fname);
  tbk_t tbk = {0};
  int n = 0;
  while(1) {
    tbf_next(tbf, &tbk);
    tbf_skip_data(&tbk);
    n++;
    if (tbk.version < 100) break;
  }
  tbf_close(tbf);
  return n;
}

static inline char* clean_path(char *path, char *fname) {
  /* expand POSIX */
  wordexp_t result;
  wordexp(path, &result, 0);
  strcpy(path, result.we_wordv[0]);
  wordfree(&result);

  /* append current directory */
  char buf[PATH_MAX];
  if (path[0] == '/') {
    strcpy(buf, path);
  } else {
    char *tmp = strdup(fname);
    strcpy(buf, dirname(tmp));
    free(tmp);
    strcat(buf, "/");
    strcat(buf, path);
  }

  return realpath(buf, NULL);
}

static inline void tbk_write_hdr(int32_t version, uint64_t dtype, int64_t n, char *msg, FILE*tbk_out) {
  char id[3] = {'t','b','k'};
  fwrite(&id,      HDR_ID,         1, tbk_out);
  fwrite(&version, HDR_VERSION,    1, tbk_out);
  fwrite(&dtype,   HDR_DATA_TYPE,  1, tbk_out);
  fwrite(&n,       HDR_NMAX,       1, tbk_out);
  fwrite(msg,      HDR_EXTRA,      1, tbk_out);
}

typedef struct conf_pack_t {
  float nan;
} conf_pack_t;

typedef struct beddata_t {
  char *s[5];              /* to allow maximum 5 columns */
  int n;
} beddata_t;

void tbk_write(beddata_t *bd, uint64_t dtype, FILE *out, int n, uint8_t *aux,
               FILE*tmp_out, uint64_t *tmp_out_offset, conf_pack_t *conf);


static inline void tbk_close(tbk_t *tbk) {
  tbf_close(tbk->tbf);
  char *sname = tbk->sname;
  /* reset everything except fname and sname */
  memset(tbk, 0, sizeof(tbk_t));
  tbk->sname = sname;
}

typedef struct view_conf_t {
  int precision;
  int column_name;
  int print_all;             /* print additional column from index  */
  int print_all_units;       /* print 2nd slot for float.float and float.int */
  int na_for_negative;
  char *na_token;
  int show_unaddressed;
  int chunk_read;
  int n_chunk_index;
  int n_chunk_data;
  float max_pval;               /* maximum pval for float.float */
  int min_coverage;             /* minimum coverage for float.int */
  int full_path_as_colname;
} view_conf_t;

typedef struct tbk_data_t {
  uint64_t dtype;
  void *data;
  int n;
} tbk_data_t;

int chunk_query_region(char *fname, char **regs, int nregs, tbk_t *tbks, int n_tbks, view_conf_t *conf, FILE *out_fh);

static inline void tbk_print_columnnames(
  tbk_t *tbks, int n_tbks, int nfields, FILE *out_fh, view_conf_t *conf) {

  int ii;
  fputs("seqname\tstart\tend", out_fh);
  if (conf->print_all) {
    fputs("\toffset", out_fh);
    for(ii=4; ii<nfields; ++ii)
      fprintf(out_fh, "\tField_%d", ii+1);
  }

  for(ii=0; ii<n_tbks; ++ii) {
    fprintf(out_fh, "\t%s", tbks[ii].sname);
    if (conf->print_all_units) {
      char *tmp = malloc(strlen(tbks[ii].sname) + 5);
      strcpy(tmp, tbks[ii].sname);
      strcat(tmp, "_sig2");
      fprintf(out_fh, "\t%s", tmp);
      free(tmp);
    }
  }

  fputc('\n', out_fh);
}

#endif /* _TBMATE_H */
