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
#include <inttypes.h>
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
  fprintf(stderr, "Usage: tbmate compress [options] <in.bed> <out.tbk>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  /* fprintf(stderr, "    -s        number of bytes for each record. Will be inferred if not give.\n"); */
  fprintf(stderr, "    -s        int2, int32, int, float, double, ones ([-1,1] up to 3e-5 precision)\n");
  fprintf(stderr, "    -x        optional output of a .bed4i file containing address for each record.\n");
  fprintf(stderr, "    -h        This help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Note, in.bed is a position sorted bed file. Column 4 will be made a .tbk file.\n");
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

data_type_t data_type(char **samples, int n_samples) {
  if (is_float(samples, n_samples)) return DT_DOUBLE;
  if (is_integer(samples, n_samples)) return DT_INT32;
  return DT_NA;
}

void tbk_write(char *s, data_type_t dt, FILE *out, int n, uint8_t *aux) {

  switch(dt) {
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
    /* fprintf(stdout, "%f\t%d\n", atof(s),d); */
    fwrite(&d, sizeof(uint16_t), 1, out);
    break;
  }
  case DT_NA: wzfatal("Fail to detect data type. Please specify -s explicity.\n");
  default: wzfatal("Unrecognized data type: %d.\n", dt);
  }
}
  
int main_pack(int argc, char *argv[]) {

  int c;
  if (argc<2) return usage();
  data_type_t dt = DT_NA;
  char *bed4i_path = NULL;
  while ((c = getopt(argc, argv, "s:x:h"))>=0) {
    switch (c) {
    case 's':
      if (strcmp(optarg, "int2") == 0) dt = DT_INT2;
      else if (strcmp(optarg, "int") == 0) dt = DT_INT32;
      else if (strcmp(optarg, "int32") == 0) dt = DT_INT32;
      else if (strcmp(optarg, "float") == 0) dt = DT_FLOAT;
      else if (strcmp(optarg, "double") == 0) dt = DT_DOUBLE;
      else if (strcmp(optarg, "ones") == 0) dt = DT_ONES;
      else wzfatal("Unrecognized data type: %s.\n", optarg);
      break;
    case 'x': bed4i_path = strdup(optarg); break;
    case 'h': return usage(); break;
    default: usage(); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  if (optind + 1 > argc) { 
    usage(); 
    wzfatal("Please supply input file.\n"); 
  }

  bed_file_t *bed = init_bed_file(argv[optind++]);
  FILE *tbk_out = NULL;
  if (optind < argc) tbk_out = fopen(argv[optind++], "wb");
    
  bed1_t *b = init_bed1(NULL, NULL);
  /* b->tid = -1; */

  char *bd;

  FILE *bed4i = NULL;
  if (bed4i_path) {
    if (strcmp(bed4i_path, "stdout") == 0) {
      bed4i = stdout;
    } else if (strcmp(bed4i_path, "stderr") == 0) {
      bed4i = stderr;
    } else {
      bed4i = fopen(bed4i_path, "w");
    }
  }
  
  int n = 0;
  char *samples[1000] = {0};
  int i;
  uint8_t aux;                  /* sub-byte encoding */
  while (bed_read1(bed, b, parse_data)) {

    if (bed4i) {
      fprintf(bed4i, "%s\t%"PRId64"\t%"PRId64"\t%d\n", b->seqname, b->beg, b->end, n);
    }
    
    bd = (char*) b->data;
    if (n < 1000) {
      samples[n++] = bd;
      continue;
    }

    if (n == 1000) {
      if (dt == DT_NA) dt = data_type(samples, n);
      if (tbk_out) fwrite(&dt, TBK_HEADER_SIZE, 1, tbk_out);
      for(i=0; i<n; ++i) {
        if (tbk_out) tbk_write(samples[i], dt, tbk_out, i, &aux);
        free(samples[i]);
      }
    }
    if (tbk_out) tbk_write(bd, dt, tbk_out, n, &aux);
    free(bd);
    n++;
  }

  /* if no more than 1000 records */
  if (n <= 1000) {
    if (dt == DT_NA) dt = data_type(samples, n);
    if (tbk_out) fwrite(&dt, TBK_HEADER_SIZE, 1, tbk_out);
    for(i=0; i<n; ++i) {
      if (tbk_out) tbk_write(samples[i], dt, tbk_out, i, &aux);
      free(samples[i]);
    }
  }

  if (dt == DT_INT2) { if (tbk_out) fwrite(&aux, 1, 1, tbk_out); aux=0; }
  
  free_bed1(b, NULL);
  free_bed_file(bed);
  if (bed4i) {
    fclose(bed4i);
    free(bed4i_path);
  }
  if (tbk_out) fclose(tbk_out);
  return 0;
}
