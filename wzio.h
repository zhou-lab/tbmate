#ifndef _WZIO_H
#define _WZIO_H

#include <zlib.h>
#include "wzmisc.h"
#include "wvec.h"

/*******************************
 ** Open file for reading and **
 ** error handling            **
 *******************************/
static inline gzFile wzopen(char *path) {
  gzFile fh;
  if (strcmp(path, "-") == 0) {
    fh = gzdopen(fileno(stdin), "r");
  } else {
    fh = gzopen(path, "r");
    if (!fh) {
      fprintf(stderr, "[%s:%d] Fatal, cannot open file: %s\n",
              __func__, __LINE__, path);
      fflush(stderr);
      exit(1);
    }
  }
  return fh;
}

static inline FILE *wzopen_out(char *path) {
   FILE *out;
   if (path) {
      out = fopen(path, "w");
      if (!out) {
         fprintf(stderr, "[%s:%d] Fatal, cannot open file: %s\n",
                 __func__, __LINE__, path);
         fflush(stderr);
         exit(1);
      }
   } else {
      out = stdout;
   }
   return out;
}

#define wzclose gzclose

/*****************************
 ** Read one line from file **
 *****************************

 * Usage:
 * char *line;
 * gzFile_read_line(fh, &line);
 *
 * "*s" is either NULL or 
 * previously allocated c-string
 * returns 1 if hitting \n 0 if EOF */
static inline int gzFile_read_line(gzFile fh, char **s) {

  if (s == NULL) {
    fprintf(stderr, "[%s:%d] Fatal, empty string construct.\n", __func__, __LINE__);
    fflush(stderr);
    exit(1);
  }
  
  /* reset s */
  int m = 10, l = 0;            /* memory and string length */
  *s = realloc(*s, m);

  /* read until '\n' or EOF */
  while (1) {
    int c = gzgetc(fh);
    if (l > m-2) { m <<= 1; *s = realloc(*s, m); }
    if (c == '\n') {(*s)[l] = '\0'; return 1;}
    if (c == EOF) {(*s)[l] = '\0'; return 0;}
    (*s)[l++] = c;
  }
  return 0;                     /* should not come here */
}

static inline int gzFile_count_lines(gzFile fh) {

  int n = 0;
  while(1) {
    int c = gzgetc(fh);
    if (c == '\n') n++;
    if (c == EOF) return n+1;
  }
  return 0;                     /* should not come here */
}

/****************************
 ** Get one field by index **
 ****************************
 field_index is 0-based
 result creates a new allocated object,
 return 0 if there are not enough fields, 1 if success */
static inline int line_get_field(const char *line, int field_index, const char *sep, char **field) {

  char *working = calloc(strlen(line) + 1, sizeof(char));
  strcpy(working, line);
  char *tok;

  tok = strtok(working, sep);
  int i;
  for (i=0; i<field_index; ++i)
    tok = strtok(NULL, sep);

  if (tok == NULL) {            /* not enough fields */
    free(working);
    return 0;
  }

  *field = strdup(tok);
  free(working);
  return 1;
}

/********************************
 ** Get all fields of one line **
 ********************************
Usage:
   char **fields; int nfields;
   line_get_fields("my line", " ", &fields, &nfields);
   free_fields(fields, nfields);

   Note: separators/delimiters are not merged - the most likely use-case. */
#define free_fields(fds, nfds) free_char_array(fds, nfds)
static inline void line_get_fields(const char *line, const char *sep, char ***fields, int *nfields) {

  *nfields = 1;
  const char *s = line;
  while ((s = strpbrk(s, sep)) != NULL) { (*nfields)++; s++; }

  *fields = calloc(*nfields, sizeof(char *));
  char *working = calloc(strlen(line) + 1, sizeof(char));
  strcpy(working, line);
  char *tok; int i;

  tok = strtok(working, sep);
  for (i=0; tok != NULL; ++i) {
    (*fields)[i] = strdup(tok);
    tok = strtok(NULL, sep);
  }
  free(working);
}


static inline void line_get_fields2(
  const char *line, const char *sep, char ***fields, int *nfields, char **aux) {

  int nfields_ = 1;
  const char *s = line;
  while ((s = strpbrk(s, sep)) != NULL) { nfields_++; s++; }

  if (*nfields < 0) {
    *nfields = nfields_;
    *fields = calloc(*nfields, sizeof(char*));
  } else if (*nfields != nfields_) {
    fprintf(stderr, "Wrong field number %d (expecting %d).\n", nfields_, *nfields);
    fflush(stderr);
    exit(1);
  }

  *aux = realloc(*aux, (strlen(line) + 1) * sizeof(char));
  strcpy(*aux, line);
  char *tok; int i;

  tok = strtok(*aux, sep);
  for (i=0; tok != NULL; ++i) {
    (*fields)[i] = realloc((*fields)[i], strlen(tok)+1);
    strcpy((*fields)[i], tok);
    tok = strtok(NULL, sep);
  }
}

#endif /* _WZIO_H */
