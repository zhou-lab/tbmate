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

  tbk_t tbk = {0}; int i=0;
  for (;optind < argc; optind++, i++) {
    tbk.fname = argv[optind];

    /* set message  */
    if(msg) {
      tbk_open_update(&tbk);
      fseek(tbk.fh, HDR_ID+HDR_VERSION+HDR_DATA_TYPE+HDR_MAX_OFFSET, SEEK_SET);
      /* truncate if too long */
      if (strlen(msg) >= HDR_EXTRA-1) {
        fprintf(stderr, "[Warning] Message too long, truncated to %d:\n%s", HDR_EXTRA-1, msg);
        msg[HDR_EXTRA-1] = '\0';
      }
      fwrite(msg, strlen(msg)+1, 1, tbk.fh);
      /* fwrite(msg, HDR_EXTRA, 1, tbk.fh); */
      tbk_close(&tbk);

    } else {

      tbk_open(&tbk);
      fprintf(stdout, "\nTBK File: %s\n", tbk.fname);
      fprintf(stdout, "TBK Version: %d\n", tbk.version);
      switch(DATA_TYPE(tbk.dtype)) {
      case DT_INT1:   fputs("Data type: INT1\n", stdout);    break;
      case DT_INT2:   fputs("Data type: INT2\n", stdout);    break;
      case DT_INT32:  fputs("Data type: INT32\n", stdout);   break;
      case DT_FLOAT:  fputs("Data type: FLOAT\n", stdout);   break;
      case DT_DOUBLE: fputs("Data type: DOUBLE\n", stdout);  break;
      case DT_ONES:   fputs("Data type: ONES\n", stdout);    break;
      case DT_STRINGF:fprintf(stdout, "Data type: STRINGF (n=%"PRIu64")\n", STRING_MAX(tbk.dtype)); break;
      case DT_STRINGD:fputs("Data type: STRINGD\n", stdout); break;
      default: wzfatal("Data type %"PRIu64" unrecognized.\n", tbk.dtype);
      }
      fprintf(stdout, "Number of data: %"PRId64"\n", tbk.offset_max);
      fputs("Message: ", stdout);
      if (tbk.extra[0]) fputs(tbk.extra, stdout);
      fputs("\n\n", stdout);
      tbk_close(&tbk);
    }
  }
  return 0;
}

