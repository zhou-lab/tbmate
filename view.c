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
  if (offset >= tbk->nmax) {wzfatal("Error: query %d out of range. Wrong idx file?", offset);}

  switch(DATA_TYPE(tbk->dtype)) {
  /* case DT_INT1: { */
  /*   if(tbk->offset == 0 || offset/8 != tbk->offset-1) { /\* need to read? *\/ */
  /*     if(offset/8 != tbk->offset) {                     /\* need to seek? *\/ */
  /*       tbk->offset = offset/8; */
  /*       if(fseek(tbk->fh, offset/8+HDR_TOTALBYTES, SEEK_SET)) */
  /*         wzfatal("File %s cannot be seeked.\n", tbk->fname); */
  /*     } */
  /*     fread(&(tbk->data), 1, 1, tbk->fh); tbk->offset++; */
  /*   } */
  /*   fprintf(out_fh, "\t%d", ((tbk->data)>>(offset%8)) & 0x1); */
  /*   break; */
  /* } */
  /* case DT_INT2: { */
  /*   if(tbk->offset == 0 || offset/4 != tbk->offset-1) { /\* need to read? *\/ */
  /*     if(offset/4 != tbk->offset) {                       /\* need to seek? *\/ */
  /*       tbk->offset = offset/4; */
  /*       if(fseek(tbk->fh, offset/4+HDR_TOTALBYTES, SEEK_SET)) */
  /*         wzfatal("File %s cannot be seeked.\n", tbk->fname); */
  /*     } */
  /*     fread(&(tbk->data), 1, 1, tbk->fh); tbk->offset++; */
  /*   } */
  /*   fprintf(out_fh, "\t%d", ((tbk->data)>>((offset%4)*2)) & 0x3); */
  /*   break; */
  /* } */
  case DT_INT32: {
    tbk_seek_n(tbk, offset);
    int data;
    tbf_read(tbk->tbf, &data, 4, 1);
    if (conf->na_for_negative && data < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else fprintf(out_fh, "\t%d", data);
    break;
  }
  case DT_FLOAT: {
    tbk_seek_n(tbk, offset);    
    float data;
    tbf_read(tbk->tbf, &data, 4, 1);
    if (conf->na_for_negative && data < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else fprintf(out_fh, "\t%f", data);
    break;
  }
  case DT_DOUBLE: {
    tbk_seek_n(tbk, offset);
    double data;
    tbf_read(tbk->tbf, &data, 8, 1);
    if (conf->na_for_negative && data < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else fprintf(out_fh, "\t%f", data);
    break;
  }
  case DT_STRINGF: {
    tbk_seek_n(tbk, offset);
    
    uint64_t n = STRING_MAX(tbk->dtype);
    if (!*aux) *aux = malloc(n+1);
    tbf_read(tbk->tbf, *aux, 1, n);
    if ((*aux)[n-1] != '\0') (*aux)[n] = '\0';
    fputc('\t', out_fh);
    fputs(*aux, out_fh);
    break;
  }
  case DT_STRINGD: {
    tbk_seek_n(tbk, offset);

    int64_t string_offset;
    tbf_read(tbk->tbf, &string_offset, 8, 1);
    tbk_seek_offset(tbk, string_offset + tbk->nmax*8);

    kstring_t ks = {0};
    char c;
    while (1) {
      tbf_read(tbk->tbf, &c, 1, 1);
      if (c) kputc(c, &ks);
      else break;
    }
    if (ks.s) { fputc('\t', out_fh); fputs(ks.s, out_fh); }
    free(ks.s);
    break;
  }
  case DT_ONES: {
    tbk_seek_n(tbk, offset);
    
    uint16_t data;
    tbf_read(tbk->tbf, &data, 2, 1);
    float dataf = uint16_to_float(data);
    if (conf->na_for_negative && dataf < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else fprintf(out_fh, "\t%.*f", conf->precision, dataf);
    /* fprintf(out_fh, "\t%d", data); */
    break;
  }
  case DT_FLOAT_INT: {
    tbk_seek_n(tbk, offset);
    
    float data; int data2;
    tbf_read(tbk->tbf, &data, 4, 1);
    tbf_read(tbk->tbf, &data2, 4, 1);

    if (conf->na_for_negative && data < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else {
      if (conf->min_coverage >= 0 && data2 < conf->min_coverage) {
        fputc('\t', out_fh); fputs(conf->na_token, out_fh);
      } else {
        fprintf(out_fh, "\t%f", data);
      }
    }
    if (conf->print_all_units) fprintf(out_fh, "\t%d", data2);
    break;
  }
  case DT_FLOAT_FLOAT: {
    tbk_seek_n(tbk, offset);
    
    float data,  data2;
    tbf_read(tbk->tbf, &data, 4, 1);
    tbf_read(tbk->tbf, &data2, 4, 1);
    
    if (conf->na_for_negative && data < 0) {
      fputc('\t', out_fh); fputs(conf->na_token, out_fh);
    } else {
      if (conf->max_pval >= 0 && data2 > conf->max_pval) {
        fputc('\t', out_fh); fputs(conf->na_token, out_fh);
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

static int query_regions(
  char *fname, char **regs, int nregs,
  tbk_t *tbks, int n_tbks, view_conf_t *conf, FILE *out_fh) {
  
  int i;
  htsFile *fp = hts_open(fname,"r");
  if(!fp) error("Could not read %s\n", fname);

  tbx_t *tbx = tbx_index_load(fname);
  if(!tbx) error("Could not load .tbi/.csi index of %s\n", fname);
  kstring_t str = {0,0,0};
  const char **seq = NULL;
  
  char **fields = NULL;
  int nfields = -1;
  char *aux = NULL; char *aux2 = NULL;
  
  int offset, ii, k;
  int linenum=0;
  for(i=0; i<nregs; i++) {
    hts_itr_t *itr = tbx_itr_querys(tbx, regs[i]);
    if(!itr) continue;
    while (tbx_itr_next(fp, tbx, itr, &str) >= 0) {

      line_get_fields2(str.s, "\t", &fields, &nfields, &aux);
      
      if (nfields < 3)
        wzfatal("[%s:%d] Bed file has fewer than 3 columns.\n", __func__, __LINE__);

      if (!linenum && conf->column_name) {
        tbk_print_columnnames(tbks, n_tbks, nfields, out_fh, conf);
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
  fprintf(stderr, "    -l        provide tbk file names in the list.\n");
  fprintf(stderr, "    -g        REGION\n");
  fprintf(stderr, "    -c        print column name\n");
  fprintf(stderr, "    -F        show full path as column name, otherwise base name.\n");
  fprintf(stderr, "    -a        print all column in the index.\n");
  fprintf(stderr, "    -b        print additional column for float.float and float.int.\n");
  fprintf(stderr, "    -d        using NA for negative values\n");
  fprintf(stderr, "    -N        NA token [%s]\n", conf->na_token);
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

static void parse_tbf_from_argument(
  int argc, char **argv, int optind,
  tbf_t **tbfs, int *n_tbfs) {
  
  int i;
  struct dirent *dir;
  char *fname;
  for(i=0; optind < argc; optind++, i++) {
    /* if it's a directory then list all the .tbk files inside */
    DIR* d = opendir(argv[optind]);
    if (d) { /* Directory exists */
      while ((dir = readdir(d)) != NULL) {
        if (strlen(dir->d_name) > 4 &&
            strcmp(dir->d_name + strlen(dir->d_name) - 4, ".tbk")==0) {

          /* identify actual file name */
          fname = malloc(strlen(argv[optind]) + strlen(dir->d_name) + 5);
          strcpy(fname, argv[optind]);
          strcat(fname, "/");
          strcat(fname, dir->d_name);

          (*tbfs) = realloc((*tbfs), (++(*n_tbfs)) * sizeof(tbf_t));
          tbf_open1(fname, &(*tbfs)[*n_tbfs-1], NULL);
          free(fname);
        }
      }
      closedir(d);
    } else {
      (*tbfs) = realloc((*tbfs), (++(*n_tbfs)) * sizeof(tbf_t));
      tbf_open1(argv[optind], &(*tbfs)[*n_tbfs-1], NULL);
    }
  }
}

static void parse_tbf_fname_list(
  char *tbk_fname_list,
  tbf_t **tbfs, int *n_tbfs) {
  
  if (tbk_fname_list == NULL) return;
  
  gzFile fh = wzopen(tbk_fname_list);
  if (!fh) { wzfatal("Cannot read tbk file name list."); }
  char *line = NULL; char **fields; int nfields; char *sname;
  while(gzFile_read_line(fh, &line)) {
    line_get_fields(line, "\t", &fields, &nfields);
    if (nfields > 0) {
      (*tbfs) = realloc((*tbfs), (++(*n_tbfs)) * sizeof(tbf_t));
      if (nfields > 1) sname = fields[1]; else sname = NULL;
      tbf_open1(fields[0], &(*tbfs)[*n_tbfs-1], sname);
    }
    free_fields(fields, nfields);
  }
  free(line);
}

void parse_tbk_from_tbf(tbf_t *tbf, tbk_t **tbks, int *n_tbks) {
  tbk_t *tbk;
  int n = 0;
  while (1) {
    (*tbks) = realloc((*tbks), (++(*n_tbks))*sizeof(tbk_t));
    tbk = &(*tbks)[(*n_tbks)-1];
    tbf_next(tbf, tbk);
    n++;
    if (tbf->sname_first != NULL) tbk->sname = strdup(tbf->sname_first);
    if (tbk->version < 100) break;
    tbf_skip_data(tbk);
  }

  if (n==1 && tbk->sname == NULL) {
    tbk_set_sname_by_fname(&(*tbks)[(*n_tbks)-1]);
  }
}

static void infer_idx(tbk_t *tbks, int n_tbks, char **idx_fname) {

  if (*idx_fname != NULL) return;
  
  /* look at the message box for idx_fname */
  int i;
  for(i=0; i<min(n_tbks, 500); ++i) {
    char *res = clean_path(tbks[i].extra, tbks[i].tbf->fname);
    if (res) {
      DIR *d = opendir(res);
      if (d) {               /* exclude the possibility that it's a folder */
        closedir(d);
      } else {
        /* FILE *fp = fopen(res,"r"); */
        htsFile *fp = hts_open(res,"r");
        if(fp) {
          *idx_fname = res;
          hts_close(fp);
          break;
        }
      }
    }
  }

  /* Last resort:
     look at the parent folder for the first tbk file idx.gz */
  if (*idx_fname == NULL) {
    char *tmp = strdup(tbks[0].tbf->fname); /* search first tbk */
    char *dir = dirname(tmp);
    *idx_fname = calloc(strlen(dir) + 10, sizeof(char));
    strcpy(*idx_fname, dir);
    strcat(*idx_fname, "/idx.gz");
    free(tmp);
  }
}

int main_view(int argc, char *argv[]) {

  view_conf_t conf = {0};
  conf.precision = 3;
  conf.print_all = 0;
  conf.print_all_units = 0;
  conf.column_name = 0;
  conf.na_for_negative = 0;
  conf.show_unaddressed = 0;
  conf.chunk_read = 0;
  conf.n_chunk_index = 1000000;
  conf.n_chunk_data = 1000000;
  conf.max_pval = -1.0;
  conf.min_coverage = -1;
  conf.full_path_as_colname = 0;
  conf.na_token = strdup("NA");
  
  int c;
  if (argc<2) return usage(&conf);

  char *regions_fname = NULL;
  char *region = NULL;
  FILE *out_fh = stdout;
  char *idx_fname = NULL;
  char *tbk_fname_list = NULL;
  while ((c = getopt(argc, argv, "i:l:o:R:N:m:n:p:g:s:t:ckabduFh"))>=0) {
    switch (c) {
    case 'i': idx_fname = strdup(optarg); break;
    case 'l': tbk_fname_list = strdup(optarg); break;
    case 'o': out_fh = fopen(optarg, "w"); break;
    case 'R': regions_fname = optarg; break;
    case 'N': conf.na_token = strdup(optarg); break;
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
    case 'd': conf.na_for_negative = 1; break;
    case 'u': conf.show_unaddressed = 1; break;
    case 'F': conf.full_path_as_colname = 1; break;
    case 'h': return usage(&conf); break;
    default: usage(&conf); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  if (optind > argc && !tbk_fname_list) {
    usage(&conf); 
    wzfatal("Please supply tbk file.\n"); 
  }

  int nregs = 0;
  char **regs = NULL;

  int n_tbks = 0; tbk_t *tbks = NULL;
  int n_tbfs = 0; tbf_t *tbfs = NULL;
  parse_tbf_from_argument(argc, argv, optind, &tbfs, &n_tbfs);
  parse_tbf_fname_list(tbk_fname_list, &tbfs, &n_tbfs);
  int i;
  for (i=0; i<n_tbfs; ++i) {
    parse_tbk_from_tbf(&tbfs[i], &tbks, &n_tbks);
  }
  
  infer_idx(tbks, n_tbks, &idx_fname);
  
  regs = parse_regions(regions_fname, region, &nregs);
  int ret;
  if (conf.chunk_read)
    ret = chunk_query_region(idx_fname, regs, nregs, tbks, n_tbks, &conf, out_fh);
  else
    ret = query_regions(idx_fname, regs, nregs, tbks, n_tbks, &conf, out_fh);

  if (n_tbfs > 0) {for (i=0; i<n_tbfs; ++i) tbf_close(&tbfs[i]); free(tbfs);}
  if (n_tbks > 0) {for (i=0; i<n_tbks; ++i) free(tbks[i].sname); free(tbks);}
  if (idx_fname) free(idx_fname);
  free(conf.na_token);
  if (region) free(region);
  return ret;
}

