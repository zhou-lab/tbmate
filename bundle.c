/* bundle tbk files
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

#include "tbmate.h"

static int usage() {
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: tbmate bundle [options] <out.tbk> <in1.tbk> <in2.tbk> ...\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -h        This help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Note, the first tbk maybe multiple tbk.\n");
  fprintf(stderr, "\n");

  return 1;
}

int main_bundle(int argc, char *argv[]) {

  int c;
  if (argc<3) return usage();

  while ((c = getopt(argc, argv, "h"))>=0) {
    switch (c) {
    case 'h': return usage(); break;
    default: usage(); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  FILE *out = fopen(argv[optind++], "wb");

  tbf_t tbf;
  tbk_t tbk = {0};
  for(; optind < argc; optind++) {
    tbf_open1(argv[optind], &tbf, NULL);
    while (1) {
      tbf_next(&tbf, &tbk);

      /* all except last will have triple-digit version number */
      if (optind + 1 < argc && tbk.version < 100) tbk.version = 100;
      else tbk.version = 1;

      tbk_set_sname_by_fname(&tbk);

      if (strlen(tbk.extra) + strlen(tbk.sname) >= HDR_EXTRA-5) {
        wzfatal(
          "%s index and %s sname is too long. Consider shorter sample names.",
          tbk.extra, tbk.sname);
      }
      strcpy(tbk.extra + strlen(tbk.extra) + 2, tbk.sname);
      tbk_write_hdr(tbk.version, tbk.dtype, tbk.nmax, tbk.extra, out);
    
      char data; unsigned j;
      for (j=0; j<tbk.nmax * unit_size(tbk.dtype); ++j) {
        tbf_read(tbk.tbf, &data, 1, 1);
        fwrite(&data, 1, 1, out);
      }
      tbk_close(&tbk);
      free(tbk.sname);
      
      if (tbk.version < 100) break;
    }
  }

  fclose(out);
  
  return 0;
}
