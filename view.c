/* View .tbi with .tbk
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

#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
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

static char **parse_regions(char *regions_fname, char *region, int *nregs) {
  kstring_t str = {0,0,0};
  int iseq = 0, ireg = 0;
  char **regs = NULL;
  *nregs = 0;

  if (regions_fname) {
    // improve me: this is a too heavy machinery for parsing regions...
    regidx_t *idx = regidx_init(regions_fname, NULL, NULL, 0, NULL);
    if (!idx) error("Could not build region list for \"%s\"\n", regions_fname);

    (*nregs) += regidx_nregs(idx);
    regs = (char**) malloc(sizeof(char*)*(*nregs));

    int nseq;
    char **seqs = regidx_seq_names(idx, &nseq);
    for (iseq=0; iseq<nseq; iseq++)
    {
      regitr_t itr;
      regidx_overlap(idx, seqs[iseq], 0, UINT32_MAX, &itr);
      while ( itr.i < itr.n )
      {
        str.l = 0;
        ksprintf(&str, "%s:%d-%d", seqs[iseq], REGITR_START(itr)+1, REGITR_END(itr)+1);
        regs[ireg++] = strdup(str.s);
        itr.i++;
      }
    }
    regidx_destroy(idx);
  }
  free(str.s);

  if (region) {

    char **fields; int nfields;
    line_get_fields(region, ",", &fields, &nfields);
    regs = realloc(regs, sizeof(char*)*((*nregs)+nfields));
    int i;
    for (i=0; i<nfields; ++i) {
      regs[(*nregs)+i] = fields[i];
    }
    (*nregs) += nfields;
    free(fields);
  }

  /* whole file */
  if (!*nregs) {
    regs = (char**) malloc(sizeof(char*));
    if (!regs) error(NULL);
    regs[0] = strdup(".");
    if (!regs[0]) error(NULL);
    *nregs = 1;
  }
  
  return regs;
}

void tbk_query(tbk_t *tbk, int64_t offset, view_conf_t *conf, FILE *out_fh, char **aux) {

  /* when the offset is unfound */
  if (offset < 0) { fputs("\t-1", out_fh); return; }
  if (offset >= tbk->offset_max) {wzfatal("Error: query %d out of range. Wrong idx file?", offset);}

  switch(DATA_TYPE(tbk->dtype)) {
  case DT_INT1: {
    if(tbk->offset == 0 || offset/8 != tbk->offset-1) { /* need to read? */
      if(offset/8 != tbk->offset) {                     /* need to seek? */
        tbk->offset = offset/8;
        if(fseek(tbk->fh, offset/8+HDR_TOTALBYTES, SEEK_SET))
          wzfatal("File %s cannot be seeked.\n", tbk->fname);
      }
      fread(&(tbk->data), 1, 1, tbk->fh); tbk->offset++;
    }
    fprintf(out_fh, "\t%d", ((tbk->data)>>(offset%8)) & 0x1);
    break;
  }
  case DT_INT2: {
    if(tbk->offset == 0 || offset/4 != tbk->offset-1) { /* need to read? */
      if(offset/4 != tbk->offset) {                       /* need to seek? */
        tbk->offset = offset/4;
        if(fseek(tbk->fh, offset/4+HDR_TOTALBYTES, SEEK_SET))
          wzfatal("File %s cannot be seeked.\n", tbk->fname);
      }
      fread(&(tbk->data), 1, 1, tbk->fh); tbk->offset++;
    }
    fprintf(out_fh, "\t%d", ((tbk->data)>>((offset%4)*2)) & 0x3);
    break;
  }
  case DT_INT32: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*4+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    int data;
    fread(&data, 4, 1, tbk->fh); tbk->offset++;
    if (conf->dot_for_negative && data < 0) fputs("\t.", out_fh);
    else fprintf(out_fh, "\t%d", data);
    break;
  }
  case DT_FLOAT: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*4+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    float data;
    fread(&data, 4, 1, tbk->fh); tbk->offset++;
    if (conf->dot_for_negative && data < 0) fputs("\t.", out_fh);
    else fprintf(out_fh, "\t%f", data);
    break;
  }
  case DT_DOUBLE: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*8+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    double data;
    fread(&data, 8, 1, tbk->fh); tbk->offset++;
    if (conf->dot_for_negative && data < 0) fputs("\t.", out_fh);
    else fprintf(out_fh, "\t%f", data);
    break;
  }
  case DT_STRINGF: {
    uint64_t n = STRING_MAX(tbk->dtype);
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*n+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }

    if (!*aux) *aux = malloc(n+1);
    fread(*aux, 1, n, tbk->fh); tbk->offset++;
    if ((*aux)[n-1] != '\0') (*aux)[n] = '\0';
    fputc('\t', out_fh);
    fputs(*aux, out_fh);
    break;
  }
  case DT_STRINGD: {
    if (offset != tbk->offset) {
      if(fseek(tbk->fh, offset*8+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }

    int64_t string_offset;
    fread(&string_offset, 8, 1, tbk->fh); tbk->offset++;

    if (fseek(tbk->fh, string_offset+tbk->offset_max*8+HDR_TOTALBYTES, SEEK_SET))
      wzfatal("File %s cannot be seeked.\n", tbk->fname);

    kstring_t ks = {0};
    char c;
    while (1) {
      fread(&c, 1,1,tbk->fh);
      if (c) kputc(c, &ks);
      else break;
    }
    if (ks.s) { fputc('\t', out_fh); fputs(ks.s, out_fh); }
    free(ks.s);

    tbk->offset = -1; // need to be reset
    break;
  }
  case DT_ONES: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*2+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    uint16_t data;
    fread(&data, 2, 1, tbk->fh); tbk->offset++;
    float dataf = uint16_to_float(data);
    if (conf->dot_for_negative && dataf < 0) fputs("\t.", out_fh);
    else fprintf(out_fh, "\t%.*f", conf->precision, dataf);
    /* fprintf(out_fh, "\t%d", data); */
    break;
  }
  case DT_FLOAT_INT: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*8+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    float data; int data2;
    fread(&data, 4, 1, tbk->fh); fread(&data2, 4, 1, tbk->fh); tbk->offset++;

    if (conf->dot_for_negative && data < 0) {
      fputs("\t.", out_fh);
    } else {
      if (conf->min_coverage >= 0 && data2 < conf->min_coverage) {
        fputs("\t.", out_fh);
      } else {
        fprintf(out_fh, "\t%f", data);
      }
    }
    if (conf->print_all_units) fprintf(out_fh, "\t%d", data2);
    break;
  }
  case DT_FLOAT_FLOAT: {
    if(offset != tbk->offset) {
      if(fseek(tbk->fh, offset*8+HDR_TOTALBYTES, SEEK_SET))
        wzfatal("File %s cannot be seeked.\n", tbk->fname);
      tbk->offset = offset;
    }
    
    float data,  data2;
    fread(&data, 4, 1, tbk->fh); fread(&data2, 4, 1, tbk->fh); tbk->offset++;

    if (conf->dot_for_negative && data < 0) {
      fputs("\t.", out_fh);
    } else {
      if (conf->max_pval >= 0 && data2 > conf->max_pval) {
        fputs("\t.", out_fh);
      } else {
        fprintf(out_fh, "\t%f", data);
      }
    }
    if (conf->print_all_units) fprintf(out_fh, "\t%f", data2);
    break;
  }
  default: wzfatal("Unrecognized data type: %d.\n", DATA_TYPE(tbk->dtype));
  }
}

static int query_regions(char *fname, char **regs, int nregs, tbk_t *tbks, int n_tbks, view_conf_t *conf, FILE *out_fh) {
  int i;
  htsFile *fp = hts_open(fname,"r");
  if(!fp) error("Could not read %s\n", fname);

  int k;
  for(k=0; k<n_tbks; ++k) tbk_open(&tbks[k]);

  tbx_t *tbx = tbx_index_load(fname);
  if(!tbx) error("Could not load .tbi/.csi index of %s\n", fname);
  kstring_t str = {0,0,0};
  const char **seq = NULL;
  
  char **fields = NULL;
  int nfields = -1;
  char *aux = NULL; char *aux2 = NULL;
  
  int offset, ii;
  int linenum=0;
  for(i=0; i<nregs; i++) {
    hts_itr_t *itr = tbx_itr_querys(tbx, regs[i]);
    if(!itr) continue;
    while (tbx_itr_next(fp, tbx, itr, &str) >= 0) {

      line_get_fields2(str.s, "\t", &fields, &nfields, &aux);
      
      if (nfields < 3)
        wzfatal("[%s:%d] Bed file has fewer than 3 columns.\n", __func__, __LINE__);

      if (!linenum) {
        if (conf->column_name) {
          fputs("seqname\tstart\tend", out_fh);
          if (conf->print_all) {
            fputs("\toffset", out_fh);
            for(ii=4; ii<nfields; ++ii)
              fprintf(out_fh, "\tField_%d", ii+1);
          }
        
          char *tmp = NULL; char *bname = NULL;
          char *tmp2;
          int k;
          for(ii=0; ii<n_tbks; ++ii) {
            tmp = strdup(tbks[ii].fname);
            bname = basename(tmp);
            k = strlen(bname);
            if (k>4 && bname[k-4]=='.' && bname[k-3]=='t' && bname[k-2]=='b' && bname[k-1]=='k')
              bname[k-4] = '\0';
            fprintf(out_fh, "\t%s", bname);
            if (conf->print_all_units) {
              tmp2 = malloc(strlen(bname) + 5);
              strcpy(tmp2, bname);
              strcat(tmp2, "_sig2");
              fprintf(out_fh, "\t%s", tmp2);
              free(tmp2);
            }
            free(tmp);
          }
        
          fputc('\n', out_fh);
        }
      }
      linenum++;
      
      ensure_number2(fields[3]);
      offset = atoi(fields[3]);

      if (offset >= 0 || conf->show_unaddressed) {
        fputs(fields[0], out_fh);
        fputc('\t', out_fh);
        fputs(fields[1], out_fh);
        fputc('\t', out_fh);
        fputs(fields[2], out_fh);
        if (conf->print_all) {
          for(ii=3; ii<nfields; ++ii)
            fprintf(out_fh, "\t%s", fields[ii]);
        }

        for(k=0; k<n_tbks; ++k) tbk_query(&tbks[k], offset, conf, out_fh, &aux2);
        fputc('\n', out_fh);
      }
    }
    tbx_itr_destroy(itr);
  }

  free(aux); free(aux2);
  free_fields(fields, nfields);
  free(seq);
  free(str.s);
  tbx_destroy(tbx);

  if(hts_close(fp)) error("hts_close returned non-zero status: %s\n", fname);

  for(k=0; k<n_tbks; ++k) tbk_close(&tbks[k]);

  for(i=0; i<nregs; i++) free(regs[i]);
  free(regs);
  return 0;
}

static int usage(view_conf_t *conf) {
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: tbmate view [options] [.tbk [...]]\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -o        optional file output\n");
  fprintf(stderr, "    -i        index, a tabix-ed bed file. Column 4 is the .tbk offset.\n");
  fprintf(stderr, "              if not given search for idx.gz and idx.gz.tbi in the folder\n");
  fprintf(stderr, "              containing the first tbk file.\n");
  fprintf(stderr, "    -g        REGION\n");
  fprintf(stderr, "    -c        print column name\n");
  fprintf(stderr, "    -F        show full path as column name, otherwise base name.\n");
  fprintf(stderr, "    -a        print all column in the index.\n");
  fprintf(stderr, "    -b        print additional column for float.float and float.int.\n");
  fprintf(stderr, "    -d        using dot for negative values\n");
  fprintf(stderr, "    -p        precision used to print float[%d]\n", conf->precision);
  fprintf(stderr, "    -u        show unaddressed (use -1)\n");
  fprintf(stderr, "    -R        file listing the regions\n");
  fprintf(stderr, "    -s        min coverage for float.int (%d)\n", conf->min_coverage);
  fprintf(stderr, "    -t        max p-value for float.float (%f)\n", conf->max_pval);
  fprintf(stderr, "    -k        read data in chunk\n");
  fprintf(stderr, "    -m        chunk size for index [%d], valid under -k.\n", conf->n_chunk_index);
  fprintf(stderr, "    -n        chunk size for data [%d], valid under -k.\n", conf->n_chunk_data);
  fprintf(stderr, "    -h        This help\n");
  fprintf(stderr, "\n");

  return 1;
}

int main_view(int argc, char *argv[]) {

  view_conf_t conf = {0};
  conf.precision = 3;
  conf.print_all = 0;
  conf.print_all_units = 0;
  conf.column_name = 0;
  conf.dot_for_negative = 0;
  conf.show_unaddressed = 0;
  conf.chunk_read = 0;
  conf.n_chunk_index = 1000000;
  conf.n_chunk_data = 1000000;
  conf.max_pval = -1.0;
  conf.min_coverage = -1;
  conf.full_path_as_colname = 0;
  
  int c;
  if (argc<2) return usage(&conf);

  char *regions_fname = NULL;
  char *region = NULL;
  FILE *out_fh = stdout;
  char *idx_fname = NULL;
  while ((c = getopt(argc, argv, "i:o:R:m:n:p:g:s:t:ckabduFh"))>=0) {
    switch (c) {
    case 'i': idx_fname = strdup(optarg); break;
    case 'o': out_fh = fopen(optarg, "w"); break;
    case 'R': regions_fname = optarg; break;
    case 'm': conf.n_chunk_index = atoi(optarg); break;
    case 'n': conf.n_chunk_data = atoi(optarg); break;
    case 'g': region = strdup(optarg); break;
    case 's': conf.min_coverage = atoi(optarg); break;
    case 't': conf.max_pval = atof(optarg); break;
    case 'p': conf.precision = atoi(optarg); break;
    case 'c': conf.column_name = 1; break;
    case 'k': conf.chunk_read = 1; break;
    case 'a': conf.print_all = 1; break;
    case 'b': conf.print_all_units = 1; break;
    case 'd': conf.dot_for_negative = 1; break;
    case 'u': conf.show_unaddressed = 1; break;
    case 'F': conf.full_path_as_colname = 1; break;
    case 'h': return usage(&conf); break;
    default: usage(&conf); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  if (optind > argc) { 
    usage(&conf); 
    wzfatal("Please supply tbk file.\n"); 
  }

  int nregs = 0;
  char **regs = NULL;

  int n_tbks = 0; // = argc - optind;
  tbk_t *tbks = NULL; // calloc(n_tbks, sizeof(tbk_t));
  int i;
  struct dirent *dir;
  for(i=0; optind < argc; optind++, i++) {
    /* if it's a directory then list all the .tbk files underneath */
    DIR* d = opendir(argv[optind]);
    if (d) { /* Directory exists. */
      while ((dir = readdir(d)) != NULL) {
        if (strlen(dir->d_name) > 4 && strcmp(dir->d_name + strlen(dir->d_name) - 4, ".tbk")==0) {
          tbks = realloc(tbks, (n_tbks+1)*sizeof(tbk_t));
          tbks[n_tbks].fname = malloc(strlen(argv[optind]) + strlen(dir->d_name) + 5);
          strcpy(tbks[n_tbks].fname, argv[optind]);
          strcat(tbks[n_tbks].fname, "/");
          strcat(tbks[n_tbks].fname, dir->d_name);
          n_tbks++;
        }
      }
      closedir(d);
    } else {
      tbks = realloc(tbks, (n_tbks+1)*sizeof(tbk_t));
      tbks[n_tbks++].fname = strdup(argv[optind]);
    }
  }

  /* look at the message box */
  if (!idx_fname) {
    /* try read the header names */
    for(i=0; i<n_tbks; ++i) {
      tbk_open(&tbks[i]);

      /* get real path */
      char buf[PATH_MAX];
      if (tbks[i].extra[0] == '/') {
        strcpy(buf, tbks[i].extra);
      } else {
        char *tmp = strdup(tbks[i].fname);
        strcpy(buf, dirname(tmp));
        free(tmp);
        strcat(buf, "/");
        strcat(buf, tbks[i].extra);
      }
      char *res = realpath(buf, NULL);
      if (res) {
        DIR *d = opendir(res);
        if (d) {               /* exclude the possibility that it's a folder */
          closedir(d);
        } else {
          /* FILE *fp = fopen(res,"r"); */
          htsFile *fp = hts_open(res,"r");
          if(fp) {
            idx_fname = res;
            tbk_close(&tbks[i]);
            hts_close(fp);
            break;
          }
        }
      }
      tbk_close(&tbks[i]);
    }
  }

  /* look at the first tbk folder idx.gz */
  if (!idx_fname) {
    char *tmp = strdup(tbks[0].fname); /* search first tbk */
    char *dir = dirname(tmp);
    idx_fname = calloc(strlen(dir) + 10, sizeof(char));
    strcpy(idx_fname, dir);
    strcat(idx_fname, "/idx.gz");
    free(tmp);
  }

  regs = parse_regions(regions_fname, region, &nregs);
  int ret;
  if (conf.chunk_read)
    ret = chunk_query_region(idx_fname, regs, nregs, tbks, n_tbks, &conf, out_fh);
  else
    ret = query_regions(idx_fname, regs, nregs, tbks, n_tbks, &conf, out_fh);
  
  if (n_tbks > 0) {for (i=0; i<n_tbks; ++i) free(tbks[i].fname);}
  free(tbks);
  if (idx_fname) free(idx_fname);
  return ret;
}

