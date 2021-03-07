#include "tbmate.h"


static int usage() {
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: tbmate header [options] [.tbk [...]]\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -m        set message for the first tbk (index path)\n");
  fprintf(stderr, "    -h        This help\n");
  fprintf(stderr, "\n");

  return 1;
}

int main_header(int argc, char *argv[]) {

  int c;
  if (argc<1) return usage();

  char *msg = NULL;
  while ((c = getopt(argc, argv, "m:h"))>=0) {
    switch (c) {
    case 'm': msg = optarg; break;
    case 'h': return usage(); break;
    default: usage(); wzfatal("Unrecognized option: %c.\n", c);
    }
  }

  if (optind > argc) { 
    usage(); 
    wzfatal("Please supply tbk file.\n"); 
  }

  tbf_t *tbf = NULL; tbk_t tbk = {0}; int i=0; int j=0;
  for (;optind < argc; optind++, i++) {
    /* set message  */
    if(msg) {
      
      tbf = tbf_open_update(argv[optind]);
      fseek(tbf->fh, HDR_ID+HDR_VERSION+HDR_DATA_TYPE+HDR_NMAX, SEEK_SET);
      /* truncate if too long */
      if (strlen(msg) >= HDR_EXTRA-1) {
        fprintf(stderr, "[Warning] Message too long, truncated to %d:\n%s", HDR_EXTRA-1, msg);
        msg[HDR_EXTRA-1] = '\0';
      }
      fwrite(msg, strlen(msg)+1, 1, tbf->fh);
      tbf_close(tbf);

    } else {

      tbf = tbf_open(argv[optind]);
      fprintf(stdout, "\nTBK File: %s\n", tbf->fname);
      fprintf(stderr, "Number of sampels: %d\n", tbf_num_samples(argv[optind]));

      j = 1;
      while (1) {
        tbf_next(tbf, &tbk);
        fprintf(stdout, "  Sample %d: %s\n", j++, tbk.sname);
        fprintf(stdout, "  TBK Version: %d\n", tbk.version);
        fputs("  Data type: ", stdout);
        switch(DATA_TYPE(tbk.dtype)) {
        case DT_INT1:        fputs("INT1\n", stdout);        break;
        case DT_INT2:        fputs("INT2\n", stdout);        break;
        case DT_INT32:       fputs("INT32\n", stdout);       break;
        case DT_FLOAT:       fputs("FLOAT\n", stdout);       break;
        case DT_DOUBLE:      fputs("DOUBLE\n", stdout);      break;
        case DT_STRINGF:     fprintf(stdout, "STRINGF (n=%"PRIu64")\n", STRING_MAX(tbk.dtype)); break;
        case DT_STRINGD:     fputs("STRINGD\n", stdout);     break;
        case DT_ONES:        fputs("ONES\n", stdout);        break;
        case DT_FLOAT_INT:   fputs("FLOAT.INT\n", stdout);   break;
        case DT_FLOAT_FLOAT: fputs("FLOAT.FLOAT\n", stdout); break;
        default: wzfatal("  Data type %"PRIu64" unrecognized.\n", tbk.dtype);
        }
        fprintf(stdout, "  Number of data: %"PRId64"\n", tbk.nmax);
        fputs("  Message: ", stdout);
        if (tbk.extra[0]) fputs(tbk.extra, stdout);
        fputs("\n\n", stdout);
        if (tbk.version < 100) break;
        tbf_skip_data(&tbk);
      }

      tbf_close(tbf);
    }
  }
  return 0;
}

