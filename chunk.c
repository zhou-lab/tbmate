/* Chunk view .tbk
 * 
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
 * 
**/

#include <inttypes.h>
#include <dirent.h>
#include "tbmate.h"
#include "wzmisc.h"
#include "wzio.h"
#include "wzbed.h"
#include "htslib/htslib/tbx.h"
#include "htslib/htslib/hts.h"
#include "htslib/htslib/regidx.h"
#include "htslib/htslib/kstring.h"

static void error(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

/* output the i-th data entry */
void tbk_print1(tbk_data_t *d, int i, view_conf_t *conf, kstring_t *ks) {

  switch(DATA_TYPE(d->dtype)) {
  case DT_INT1: {
    ksprintf(ks, "\t%d", ((uint8_t*)(d->data))[i]);
    break;
  }
  case DT_INT2: {
    ksprintf(ks, "\t%d", ((uint8_t*)(d->data))[i]);
    break;
  }
  case DT_INT32: {
    int data = ((int32_t*) (d->data))[i];
    if (conf->na_for_negative && data < 0) { kputc('\t', ks); kputs(conf->na_token, ks); }
    else ksprintf(ks, "\t%d", data);
    break;
  }
  case DT_FLOAT: {
    float data = ((float*) (d->data))[i];
    if (conf->na_for_negative && data < 0) { kputc('\t', ks); kputs(conf->na_token, ks); }
    else ksprintf(ks, "\t%f", data);
    break;
  }
  case DT_DOUBLE: {
    double data = ((double*) (d->data))[i];
    if (conf->na_for_negative && data < 0) { kputc('\t', ks); kputs(conf->na_token, ks); }
    else ksprintf(ks, "\t%f", data);
    break;
  }
  case DT_STRINGF: {
    char *data = ((char*) (d->data)) + i*STRING_MAX(d->dtype);
    kputc('\t', ks); kputs(data, ks);
    break;
  }
  case DT_STRINGD: {
    char *data = ((char**) (d->data))[i];
    kputc('\t', ks); kputs(data, ks);
    free(data);
    break;
  }
  case DT_ONES: {
    float data = ((float*) (d->data))[i];
    if (conf->na_for_negative && data < 0) { kputc('\t', ks); kputs(conf->na_token, ks); }
    else ksprintf(ks, "\t%.*f", conf->precision, data);
    break;
  }
  case DT_FLOAT_INT: {
    float data = ((float*) (d->data))[i*2];
    int data2 = ((int32_t*) (d->data))[i*2+1];
    if (conf->na_for_negative && data < 0) {
      kputc('\t', ks); kputs(conf->na_token, ks);
    } else {
      if (conf->min_coverage >= 0 && data2 < conf->min_coverage) {
        kputc('\t', ks); kputs(conf->na_token, ks);
      } else {
        ksprintf(ks, "\t%f", data);
      }
    }
    if (conf->print_all_units) ksprintf(ks, "\t%d", data2);
    break;
  }
  case DT_FLOAT_FLOAT: {
    float data = ((float*) (d->data))[i*2];
    float data2 = ((float*) (d->data))[i*2+1];
    if (conf->na_for_negative && data < 0) {
      kputc('\t', ks); kputs(conf->na_token, ks);
    } else {
      if (conf->max_pval >= 0 && data2 > conf->max_pval) {
        kputc('\t', ks); kputs(conf->na_token, ks);
      } else {
        ksprintf(ks, "\t%f", data);
      }
    }
    if (conf->print_all_units) ksprintf(ks, "\t%f", data2);
    break;
  }
  default: wzfatal("Unrecognized data type: %d.\n", DATA_TYPE(d->dtype));
  }
}

void tbk_query_n(tbk_t *tbk, int64_t chunk_beg, int n, tbk_data_t *data) {
  if (chunk_beg >= tbk->nmax) {wzfatal("Error: query %d out of range. Wrong idx file?", chunk_beg);}
  if (chunk_beg + n >= tbk->nmax) {
    n = tbk->nmax - chunk_beg;
  }
  if (n <= 0) return;
  
  data->n = n;
  data->dtype = tbk->dtype;

  switch(DATA_TYPE(tbk->dtype)) {
  /* case DT_INT1: { */
  /*   tbk->offset = offset/8;                           /\* always seek *\/ */
  /*   if(fseek(tbk->fh, offset/8+HDR_TOTALBYTES, SEEK_SET)) wzfatal("File %s cannot be seeked.\n", tbk->fname); */
    
  /*   int nb = (offset + n) / 8 - offset / 8 + 1; */
  /*   uint8_t *tmp = calloc(nb, 1); */
  /*   fread(tmp, 1, nb, tbk->fh); tbk->offset += n; */
  /*   data->data = realloc(data->data, n); */
  /*   int i; */
  /*   for (i=0; i<n; ++i) { */
  /*     ((uint8_t*)data->data)[i] = (tmp[(offset+i)/8 - offset/8] >> (offset+i)%8) & 0x1; */
  /*   } */
  /*   break; */
  /* } */
  /* case DT_INT2: { */
  /*   tbk->offset = offset/4;                           /\* always seek *\/ */
  /*   if(fseek(tbk->fh, offset/4+HDR_TOTALBYTES, SEEK_SET)) wzfatal("File %s cannot be seeked.\n", tbk->fname); */
    
  /*   int nb = (offset + n) / 4 - offset / 4; */
  /*   uint8_t *tmp = calloc(nb, 1); */
  /*   fread(tmp, 1, nb, tbk->fh); tbk->offset += n; */
  /*   data->data = realloc(data->data, n); */
  /*   int i; */
  /*   for (i=0; i<n; ++i) { */
  /*     ((uint8_t*)data->data)[i] = (tmp[(offset+i)/4 - offset/4] >> (((offset+i)%4)*2)) & 0x3; */
  /*   } */
  /*   break; */
  /* } */
  case DT_INT32: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 4*n);
    tbf_read(tbk->tbf, data->data, 4, n);
    break;
  }
  case DT_FLOAT: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 4*n);
    tbf_read(tbk->tbf, data->data, 4, n);
    break;
  }
  case DT_DOUBLE: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 8*n);
    tbf_read(tbk->tbf, data->data, 8, n);
    break;
  }
  case DT_STRINGF: {
    int smax = STRING_MAX(tbk->dtype);
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, n*smax);
    tbf_read(tbk->tbf, data->data, 1, n*smax);
    break;
  }
  case DT_STRINGD: {
    tbk_seek_n(tbk, chunk_beg);

    data->data = realloc(data->data, sizeof(char*)*n);
    uint64_t *string_offsets = malloc(sizeof(uint64_t)*n);
    tbf_read(tbk->tbf, string_offsets, 8, n);

    int i;
    for (i=0; i<n; ++i) {
      tbk_seek_offset(tbk, string_offsets[i]+tbk->nmax*8);
      kstring_t ks = {0}; char c;
      while (1) {
        tbf_read(tbk->tbf, &c, 1, 1);
        if (c) kputc(c, &ks);
        else break;
      }
      ((char**) data->data)[i] = ks.s;
    }
    free(string_offsets);
    break;
  }
  case DT_ONES: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 2*n);
    uint16_t *tmp = calloc(n, 2);
    tbf_read(tbk->tbf, tmp, 2, n);
    int ii;
    for (ii=0; ii<n; ++ii) ((float*)data->data)[ii] = uint16_to_float(tmp[ii]);
    free(tmp);
    break;
  }
  case DT_FLOAT_INT: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 8*n);
    tbf_read(tbk->tbf, data->data, 8, n);
    break;
  }
  case DT_FLOAT_FLOAT: {
    tbk_seek_n(tbk, chunk_beg);
    data->data = realloc(data->data, 8*n);
    tbf_read(tbk->tbf, data->data, 8, n);
    break;
  }
  default: wzfatal("Unrecognized data type: %d.\n", DATA_TYPE(tbk->dtype));
  }
}

/* process one chunk */
static void query_one_chunk(int *offsets, int n_offsets, tbk_t *tbks, int n_tbks, view_conf_t *conf, kstring_t *ks_out, FILE*out_fh) {

  if (n_offsets == 0) return;
  
  int k;
  tbk_data_t data = {0};
  for(k=0; k<n_tbks; ++k) {     /* iterate through samples */
    int chunk_beg = 0;
    for (chunk_beg = 0; chunk_beg < tbks[k].nmax; chunk_beg += conf->n_chunk_data) { /* iterate data chunk */
      int chunk_end = (chunk_beg + conf->n_chunk_data > tbks[k].nmax) ? tbks[k].nmax : (chunk_beg + conf->n_chunk_data);
      tbk_query_n(&tbks[k], chunk_beg, conf->n_chunk_data, &data);
      if (chunk_end - chunk_beg != data.n) {
        wzfatal("Unequal number of records read %d, expecting %d\n", data.n, chunk_end-chunk_beg);
      }

      /* save to output */
      int i;
      for(i=0; i<n_offsets; ++i) {
        if (offsets[i] >= chunk_beg && offsets[i] < chunk_end) {
          tbk_print1(&data, offsets[i]-chunk_beg, conf, &ks_out[i]);
        }
      }
    }
  }

  /* output and reset */
  int i;
  for (i=0; i < n_offsets; ++i) {
    if (ks_out[i].s) {
      if (offsets[i] >= 0) {
        fputs(ks_out[i].s, out_fh); fputc('\n', out_fh);
      } else if (conf->show_unaddressed) {
        fputs(ks_out[i].s, out_fh);
        for (k=0; k<n_tbks; ++k) fputs("\t-1", out_fh);
        fputc('\n', out_fh);
      }
      free(ks_out[i].s); memset(&ks_out[i], 0 , sizeof(kstring_t));
    }
  }
  memset(offsets, 0, sizeof(int) * conf->n_chunk_index);
}

int chunk_query_region(char *fname, char **regs, int nregs, tbk_t *tbks, int n_tbks, view_conf_t *conf, FILE *out_fh) {

  int i;
  htsFile *fp = hts_open(fname,"r");
  if(!fp) error("Could not read %s\n", fname);
  
  tbx_t *tbx = tbx_index_load(fname);
  if(!tbx) error("Could not load .tbi/.csi index of %s\n", fname);
  kstring_t str = {0,0,0};
  const char **seq = NULL;

  /* line reading and splitting */
  char **fields = NULL;
  int nfields = -1;
  char *aux = NULL;
  
  int n, ii;
  int linenum=0;

  int index_chunk_beg = 0;
  int index_chunk_end = 1;

  /* offsets in the current chunk */
  int *ns = calloc(conf->n_chunk_index, sizeof(int));
  /* output */
  kstring_t *ks_out = calloc(conf->n_chunk_index, sizeof(kstring_t));
  
  for(i=0; i<nregs; i++) {
    hts_itr_t *itr = tbx_itr_querys(tbx, regs[i]);
    if(!itr) continue;
    while (tbx_itr_next(fp, tbx, itr, &str) >= 0) {

      line_get_fields2(str.s, "\t", &fields, &nfields, &aux);

      if (nfields < 3)
        wzfatal("[%s:%d] Bed file has fewer than 3 columns.\n", __func__, __LINE__);

      if (!linenum && conf->column_name) { /* header */
        tbk_print_columnnames(tbks, n_tbks, nfields, out_fh, conf);
      }
      
      ensure_number2(fields[3]);
      n = atoi(fields[3]);

      if (n >= 0 || conf->show_unaddressed) {
        kstring_t *ks = &ks_out[index_chunk_end - index_chunk_beg - 1];
        kputs(fields[0], ks); kputc('\t', ks);
        kputs(fields[1], ks); kputc('\t', ks);
        kputs(fields[2], ks);
        if (conf->print_all) {
          for (ii=3; ii<nfields; ++ii)
            ksprintf(ks, "\t%s", fields[ii]);
        }
        ns[index_chunk_end - index_chunk_beg - 1] = n;
      
        if (index_chunk_end % conf->n_chunk_index == 0) {
          query_one_chunk(ns, index_chunk_end-index_chunk_beg, tbks, n_tbks, conf, ks_out, out_fh);
          index_chunk_beg = index_chunk_end;
        }
                
        index_chunk_end++;
      }
      
      linenum++;
    }
    tbx_itr_destroy(itr);
  }

  query_one_chunk(ns, index_chunk_end-index_chunk_beg-1, tbks, n_tbks, conf, ks_out, out_fh);

  free_fields(fields, nfields);
  free(aux);
  free(ns);
  free(seq);
  free(str.s);
  tbx_destroy(tbx);

  if(hts_close(fp)) error("hts_close returned non-zero status: %s\n", fname);
  
  for(i=0; i<nregs; i++) free(regs[i]);
  free(regs);
  return 0;
}
