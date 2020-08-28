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

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#define PACKAGE_VERSION "0.1.0"

int main_pack(int argc, char *argv[]);
int main_view(int argc, char *argv[]);

static int usage()
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Program: tbmate (Tabix Mate)\n");
  fprintf(stderr, "Version: %s\n", PACKAGE_VERSION);
  fprintf(stderr, "Contact: Wanding Zhou<wanding.zhou@pennmedicine.upenn.edu>\n\n");
  fprintf(stderr, "Usage:   tbmate <command> [options]\n\n");
  fprintf(stderr, "Command:\n");
  fprintf(stderr, "     pack         pack data to tbk\n");
  fprintf(stderr, "     view         view data stored in tbk\n");
  fprintf(stderr, "\n");

  return 1;
}

int main(int argc, char *argv[]) {
  int ret;
  if (argc < 2) return usage();
  if (strcmp(argv[1], "pack") == 0) ret = main_pack(argc-1, argv+1);
  else if (strcmp(argv[1], "view") == 0) ret = main_view(argc-1, argv+1);
  else {
    fprintf(stderr, "[main] unrecognized command '%s'\n", argv[1]);
    return 1;
  }

  fflush(stdout);             /* not enough for remote file systems */
  fclose(stdout);

  return ret;
}
