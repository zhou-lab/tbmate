/* Compress .bed into .tbk
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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "tbmate.h"
#include "wzbed.h"

void parse_data(bed1_t *b, char **fields, int nfields) {
  if (nfields < 4) wzfatal("No data in column 4.\n");
  b->data = strdup(fields[3]);
}

static int usage() {
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: tbmate pack [options] <in.bed> <out.tbk>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -s        int1, int2, int32, int, float, double, stringf, stringd, ones ([-1,1] up to 3e-5 precision)\n");
  fprintf(stderr, "    -x        optional output of an index file containing address for each record.\n");
  fprintf(stderr, "    -m        optional message, it will also be used to locate index file.\n");
  fprintf(stderr, "    -h        This help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Note, in.bed is an index-ordered bed file. Column 4 will be made a .tbk file.\n");
  fprintf(stderr, "\n");

  return 1;
}

int is_integer(char **samples, int n_samples) {
  int i, j;
  for (j=0;j<n_samples;++j) {
    for (i=0;samples[j][i];++i) {
      if (!isdigit(samples[j][i]) && samples[j][i] != '-') return 0;
    }
  }
  return 1;
}

int is_float(char **samples, int n_samples) {
  int i, j;
  for (j=0; j<n_samples; ++j) {
    for (i=0;samples[j][i];++i) {
      if (!isdigit(samples[j][i]) &&
          samples[j][i] != '.' &&
          samples[j][i] != '-') return 0;
    }
  }
  return 1;
}

int data_type(char **samples, int n_samples) {
  if (is_float(samples, n_samples)) return DT_DOUBLE;
  if (is_integer(samples, n_samples)) return DT_INT32;
  return -1;
}

void tbk_write(char *s, uint64_t dtype, FILE *out, int n, uint8_t *aux, FILE *tmp_out, uint64_t *tmp_out_offset) {

  switch(DATA_TYPE(dtype)) {
  case DT_INT1: {
    uint8_t d;
    switch(*s) {
    case '0': d=0; break;
    case '1': d=1; break;
    default: wzfatal("Error, non-binary data found: %s\n", s); }
    *aux |= d << (n&0x7);
    if((n&0x7)==7) { fwrite(aux, 1, 1, out); *aux=0; }
    break;
  }
  case DT_INT2: {
    uint8_t d = atoi(s);
    *aux |= d << ((n%4)*2);
    if(n%4==3) { fwrite(aux, 1, 1, out); *aux=0; }
    break;
  }
  case DT_INT32: {
    int32_t d = atoi(s);
    fwrite(&d, sizeof(int32_t), 1, out);
    break;
  }
  case DT_FLOAT: {
    float d = atof(s);
    fwrite(&d, sizeof(float), 1, out);
    break;
  }
  case DT_DOUBLE: {
    double d = atof(s);
    fwrite(&d, sizeof(double), 1, out);
    break;
  }
  case DT_ONES: {
    uint16_t d = float_to_uint16(atof(s));
    fwrite(&d, sizeof(uint16_t), 1, out);
    break;
  }
  case DT_STRINGF: {
    unsigned n = strlen(s);
    if (n > STRING_MAX(dtype)) s[STRING_MAX(dtype)] = 0;
    fwrite(s, 1, n, out);
    char buf = '\0';
    for (;n < STRING_MAX(dtype);++n) { fwrite(&buf, 1, 1, out); }
    break;
  }
  case DT_STRINGD: {
    int n = strlen(s) + 1;
    fwrite(s, 1, n, tmp_out);
    fwrite(tmp_out_offset, 8, 1, out);
    *tmp_out_offset += strlen(s) + 1;
    break;
  }
  case DT_NA: wzfatal("Fail to detect data type. Please specify -s explicity.\n"); break;
  default: wzfatal("Unrecognized data type: %d.\n", DATA_TYPE(dtype));
  }
}
  
int main_pack(int argc, char *argv[]) {

  int c;
  if (argc<2) return usage();
  uint64_t dtype = DT_NA;
  char *idx_path = NULL;
  char msg[HDR_EXTRA] = {0};
  uint64_t max_str_length = 64;
  while ((c = getopt(argc, argv, "s:x:m:h"))>=0) {
    switch (c) {
    case 's':
      if (strcmp(optarg, "int1") == 0)         dtype = DT_INT1;
      else if (strcmp(optarg, "int2") == 0)    dtype = DT_INT2;
      else if (strcmp(optarg, "int32") == 0)   dtype = DT_INT32;
      else if (strcmp(optarg, "int") == 0)     dtype = DT_INT32;
      else if (strcmp(optarg, "float") == 0)   dtype = DT_FLOAT;
      else if (strcmp(optarg, "double") == 0)  dtype = DT_DOUBLE;
      else if (strcmp(optarg, "ones") == 0)    dtype = DT_ONES;
      else if (strcmp(optarg, "string") == 0)  dtype = DT_STRINGD;
      else if (strcmp(optarg, "stringd") == 0) dtype = DT_STRINGD; /* fixed size string */
      else if (strcmp(optarg, "stringf") == 0) dtype = DT_STRINGF;
      else wzfatal("Unrecognized data type: %s.\n", optarg);
      break;
    case 'x': idx_path = strdup(optarg); break;
    case 'i': max_str_length = atol(optarg); break;
    case 'm': {
      if (strlen(optarg) > HDR_EXTRA - 1) wzfatal("Message cannot be over %d in length.", HDR_EXTRA);
      strcpy(msg, optarg);
      break;
    }
    case 'h': return usage(); break;
    default: usage(); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  if (dtype == DT_STRINGF) {
    dtype |= (max_str_length << 8);
  }

  if (optind + 2 > argc) { 
    usage(); 
    wzfatal("Please supply input and output file.\n"); 
  }

  bed_file_t *bed = init_bed_file(argv[optind++]);
  FILE *tbk_out = NULL;
  if (optind < argc) tbk_out = fopen(argv[optind], "wb");

  char *tmp_fname = NULL;       /* temporary file holding variable length strings */
  FILE *tmp_out = NULL;
  uint64_t tmp_out_offset = 0;
  if (dtype == DT_STRINGD) {
    tmp_fname = calloc(strlen(argv[optind])+10, 1);
    strcpy(tmp_fname, argv[optind]);
    strcat(tmp_fname, "_tmp_");
    tmp_out = fopen(tmp_fname, "wb");
  }
    
  bed1_t *b = init_bed1(NULL, NULL);
  /* b->tid = -1; */

  char *bd;

  FILE *idx = NULL;
  if (idx_path) {
    if (strcmp(idx_path, "stdout") == 0) {
      idx = stdout;
    } else if (strcmp(idx_path, "stderr") == 0) {
      idx = stderr;
    } else {
      idx = fopen(idx_path, "w");
    }
  }

  int64_t n = 0;
  char *samples[1000] = {0};
  int64_t i;
  uint8_t aux;                  /* sub-byte encoding */
  while (bed_read1(bed, b, parse_data)) {

    if (idx) {
      fprintf(idx, "%s\t%"PRId64"\t%"PRId64"\t%"PRId64"\n", b->seqname, b->beg, b->end, n);
    }
    
    bd = (char*) b->data;
    if (n < 1000) {
      samples[n++] = bd;
      continue;
    }

    if (n == 1000) {
      if (dtype == DT_NA) dtype = data_type(samples, n);
      if (tbk_out) tbk_write_hdr(1, dtype, n, msg, tbk_out);
      for(i=0; i<n; ++i) {
        if (tbk_out) tbk_write(samples[i], dtype, tbk_out, i, &aux, tmp_out, &tmp_out_offset);
        free(samples[i]);
      }
    }
    if (tbk_out) tbk_write(bd, dtype, tbk_out, n, &aux, tmp_out, &tmp_out_offset);
    free(bd);
    n++;
  }

  /* if no more than 1000 records */
  if (n <= 1000) {
    if (dtype == DT_NA) dtype = data_type(samples, n);
    if (tbk_out) tbk_write_hdr(1, dtype, n, msg, tbk_out);
    for(i=0; i<n; ++i) {
      if (tbk_out) tbk_write(samples[i], dtype, tbk_out, i, &aux, tmp_out, &tmp_out_offset);
      free(samples[i]);
    }
  }

  if (dtype == DT_INT2) { if (tbk_out) fwrite(&aux, 1, 1, tbk_out); aux=0; }
  
  free_bed1(b, NULL);
  free_bed_file(bed);
  if (idx) {
    fclose(idx);
    free(idx_path);
  }

  /* the actual size */
  fseek(tbk_out, HDR_MAX_OFFSET0, SEEK_SET);
  fwrite(&n,     HDR_MAX_OFFSET, 1, tbk_out);

  if (tmp_out) {
    fclose(tmp_out);

    /* exit(1); */
    tmp_out = fopen(tmp_fname, "rb");
    fseek(tbk_out, 0, SEEK_END);
    char data;
    unsigned j;
    for (j=0; j<tmp_out_offset; ++j) {
      fread(&data, 1, 1, tmp_out);
      fwrite(&data, 1, 1, tbk_out);
    }
    fclose(tmp_out);
    unlink(tmp_fname);
    free(tmp_fname);
  }

  if (tbk_out) fclose(tbk_out);
  
  return 0;
}
