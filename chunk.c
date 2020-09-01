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

  switch(d->dtype) {
  case  DT_INT2: {
    ksprintf(ks, "\t%d", ((uint8_t*)(d->data))[i]);
    break;
  }
  case DT_INT32: {
    int data = ((int32_t*) (d->data))[i];
    if (conf->dot_for_negative && data < 0) kputs("\t.", ks);
    else ksprintf(ks, "\t%d", data);
    break;
  }
  case DT_FLOAT: {
    float data = ((float*) (d->data))[i];
    if (conf->dot_for_negative && data < 0) kputs("\t.", ks);
    else ksprintf(ks, "\t%f", data);
    break;
  }
  case DT_ONES: {
    float data = ((float*) (d->data))[i];
    if (conf->dot_for_negative && data < 0) kputs("\t.", ks);
    else ksprintf(ks, "\t%.*f", conf->precision, data);
    break;
  }
  case DT_DOUBLE: {
    double data = ((double*) (d->data))[i];
    if (conf->dot_for_negative && data < 0) kputs("\t.", ks);
    else ksprintf(ks, "\t%f", data);
    break;
  }
  default: wzfatal("Unrecognized data type: %d.\n", d->dtype);
  }
}

void tbk_query_n(tbk_t *tbk, int64_t offset, int n, tbk_data_t *data) {
  if (offset >= tbk->offset_max) {wzfatal("Error: query %d out of range. Wrong idx file?", offset);}
  if (offset + n >= tbk->offset_max) {
    n = tbk->offset_max - offset;
  }
  if (n <= 0) return;
  
  data->n = n;
  data->dtype = tbk->dt;

  switch(tbk->dt) {
  case DT_INT2: {
    tbk->offset = offset/4;                           /* always seek */
    if(fseek(tbk->fh, offset/4+HDR_TOTALBYTES, SEEK_SET)) wzfatal("File %s cannot be seeked.\n", tbk->fname);
    
    int nb = (offset + n) / 4 - offset / 4 + 1;
    uint8_t *tmp = calloc(nb, 1);
    fread(tmp, 1, nb, tbk->fh); tbk->offset += n;
    int i;
    for (i=0; i<n; ++i) ((uint8_t*)data->data)[i] = (tmp[(offset+i)/4 - offset/4] >> (((offset+i)%4)*2)) & 0x3;
    break;
  }
  case DT_INT32: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*4+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    data->data = realloc(data->data, 4*n);
    fread(data->data, 4, n, tbk->fh); tbk->offset += n;
    break;
  }
  case DT_FLOAT: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*4+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }

    data->data = realloc(data->data, 4*n);
    fread(data->data, 4, n, tbk->fh); tbk->offset += n;
    break;
  }
  case DT_ONES: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*2+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    data->data = realloc(data->data, 4*n);

    uint16_t *tmp = calloc(n, 2);
    fread(tmp, 2, n, tbk->fh); tbk->offset += n;
    int ii;
    for (ii=0; ii<n; ++ii) ((float*)data->data)[ii] = uint16_to_float(tmp[ii]);
    free(tmp);
    break;
  }
  case DT_DOUBLE: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*8+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    data->data = realloc(data->data, 8*n);
    fread(data->data, 8, n, tbk->fh); tbk->offset += n;
    break;
  }
  default: wzfatal("Unrecognized data type: %d.\n", tbk->dt);
  }
}

/* process one chunk */
static void query_one_chunk(int *offsets, int n_offsets, tbk_t *tbks,
                            int n_tbks, view_conf_t *conf, kstring_t *ks_out, FILE*out_fh) {

  if (n_offsets == 0) return;
  
  int k;
  tbk_data_t data = {0};
  for(k=0; k<n_tbks; ++k) {     /* iterate through samples */
    int chunk_beg = 0;
    for (chunk_beg = 0; chunk_beg < tbks[k].offset_max; chunk_beg += conf->n_chunk_data) { /* iterate data chunk */
      int chunk_end = (chunk_beg + conf->n_chunk_data > tbks[k].offset_max) ? tbks[k].offset_max : (chunk_beg + conf->n_chunk_data);
      tbk_query_n(&tbks[k], chunk_beg, conf->n_chunk_data, &data);
      if (chunk_end - chunk_beg != data.n) {
        wzfatal("Unequal number of records read %d, expecting %d\n", data.n, chunk_end-chunk_beg);
      }

      /* output */
      int i;
      for(i=0; i<n_offsets; ++i) {
        if (offsets[i] >= chunk_beg && offsets[i] < chunk_end) {
          tbk_print1(&data, i, conf, &ks_out[offsets[i]-chunk_beg]);
        }
      }
    }
  }

  /* output and reset */
  int i;
  for (i=0; i < n_offsets; ++i) {
    fputs(ks_out[i].s, out_fh);
    free(ks_out[i].s); memset(&ks_out[i], 0 , sizeof(kstring_t));
  }
  memset(offsets, 0, sizeof(int) * conf->n_chunk_index);
}

int chunk_query_region(char *fname, char **regs, int nregs, tbk_t *tbks, int n_tbks, view_conf_t *conf, FILE *out_fh) {

  int i;
  htsFile *fp = hts_open(fname,"r");
  if(!fp) error("Could not read %s\n", fname);
  
  int k;
  for(k=0; k<n_tbks; ++k) tbk_open(&tbks[k]);

  tbx_t *tbx = tbx_index_load(fname);
  if(!tbx) error("Could not load .tbi/.csi index of %s\n", fname);
  kstring_t str = {0,0,0};
  const char **seq = NULL;
  char **fields; int nfields = -1;
  int offset, ii;
  int linenum=0;

  int index_chunk_beg = 0;
  int index_chunk_end = 0;

  /* offsets in the current chunk */
  int *offsets = calloc(conf->n_chunk_index, sizeof(int));
  /* output */
  kstring_t *ks_out = calloc(conf->n_chunk_index, sizeof(kstring_t));
  
  for(i=0; i<nregs; i++) {
    hts_itr_t *itr = tbx_itr_querys(tbx, regs[i]);
    if(!itr) continue;
    while (tbx_itr_next(fp, tbx, itr, &str) >= 0) {

      int nfields_;
      line_get_fields(str.s, "\t", &fields, &nfields_);

      if (nfields_ < 3)
        wzfatal("[%s:%d] Bed file has fewer than 3 columns.\n", __func__, __LINE__);

      if (nfields < 0) nfields = nfields_;
      else if (nfields != nfields_)
        wzfatal("[%s:%d] Row %d has %d columns (expecting %d).\n", __func__, __LINE__, linenum+1, nfields_, nfields);
                
      if (!linenum && conf->column_name) { /* header */
        fputs("seqname\tstart\tend", out_fh);
        if (conf->print_all) {
          fputs("\toffset", out_fh);
          for(ii=4; ii<nfields; ++ii)
            fprintf(out_fh, "\tField_%d", ii+1);
        }
        
        for(ii=0; ii<n_tbks; ++ii)
          fprintf(out_fh, "\t%s", tbks[ii].fname);
        
        fputc('\n', out_fh);
      }
      
      ensure_number2(fields[3]);
      offset = atoi(fields[3]);

      if (offset >= 0 || conf->show_unaddressed) {
        kstring_t *ks = &ks_out[index_chunk_end - index_chunk_beg];
        kputs(fields[0], ks); kputc('\t', ks);
        kputs(fields[1], ks); kputc('\t', ks);
        kputs(fields[2], ks);
        if (conf->print_all) {
          for (ii=3; ii<nfields; ++i)
            ksprintf(ks, "\t%s", fields[ii]);
        }
      }
      offsets[index_chunk_end - index_chunk_beg] = offset;
      
      if ((index_chunk_end+1) % conf->n_chunk_index == 0) {
        query_one_chunk(offsets, index_chunk_end-index_chunk_beg, tbks, n_tbks, conf, ks_out, out_fh);
        index_chunk_beg = index_chunk_end + 1;
      }
                
      linenum++;
      index_chunk_end++;
      free(fields);
      /* free_fields(fields, nfields); */
    }
    tbx_itr_destroy(itr);
  }

  query_one_chunk(offsets, index_chunk_end-index_chunk_beg, tbks, n_tbks, conf, ks_out, out_fh);
  
  free(offsets);
  free(seq);
  free(str.s);
  tbx_destroy(tbx);

  if(hts_close(fp)) error("hts_close returned non-zero status: %s\n", fname);
  
  for(k=0; k<n_tbks; ++k) tbk_close(&tbks[k]);
  
  for(i=0; i<nregs; i++) free(regs[i]);
  free(regs);
  return 0;
}
