/*****************************************************************

  MAP: Nancy's Arguments Processor

  Example:

    void usage() {printf("Usage: program [-oh] <in_file> <outfile>);}
    int main(int argc, char **argv) {
      char *operation = "NOP";
      NAP_INTRO
      NAP_IF("h") usage(); NAP_FI
      NAP_IF("o") NAP_STR(operation); NAP_FI
      NAP_OUTRO(2,2)
      printf("Input file: %s", fargv[0]);
      printf("Output file: %s", fargv[1]);
      printf("Operation: %s", operation);
      return 0;
    }
}

******************************************************************/

#ifndef NG_NAP_H
#define NG_NAP_H

#include <stdio.h>
#include <stdlib.h>

#define NAP_INTRO \
  char **fargv = (char**)malloc(argc*sizeof(char*)); \
  int fargc = 0; \
  for (int argi_ = 1; argi_ < argc; argi_++) { \
    if (argv[argi_][0] == '-') {

#define NAP_OUTRO(min_fargs,max_fargs) \
      { \
        printf("Unrecognized option: %s\n", argv[argi_]); \
        exit(0); \
      } \
    } else { \
      fargv[fargc++] = argv[argi_]; \
    } \
  } \
  if (fargc < min_fargs) usage(); \
  if (max_fargs != -1 && fargc > max_fargs) usage();

#define NAP_IF(key) if (!strcmp(key,argv[argi_]+1)) {
#define NAP_FI } else
#define NAP_STR argv[argi_++]
#define NAP_INT(dst) do { dst = strtol(argv[++argi_], NULL,  0); } while(0)
#define NAP_BIN(dst) do { dst = strtol(argv[++argi_], NULL,  2); } while(0)
#define NAP_DEC(dst) do { dst = strtol(argv[++argi_], NULL, 10); } while(0)
#define NAP_HEX(dst) do { dst = strtol(argv[++argi_], NULL, 16); } while(0)


#endif
