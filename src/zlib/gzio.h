/*
 * Copyright (C) 2012 Armin Burgmeier
 */

#ifndef C4_GZIO_H
#define C4_GZIO_H

#include <zlib.h>

extern "C"
{

gzFile ZEXPORT c4_gzopen (const char* path, const char* mode);
gzFile ZEXPORT c4_gzdopen (int fd, const char* mode);
int ZEXPORT c4_gzsetparams (gzFile file, int level, int strategy);
int ZEXPORT c4_gzread (gzFile file, voidp buf, unsigned len);
int ZEXPORT c4_gzgetc(gzFile file);
int ZEXPORT c4_gzungetc(int c, gzFile file);
char * ZEXPORT c4_gzgets(gzFile file, char* buf, int len);
int ZEXPORT c4_gzwrite (gzFile file, voidpc buf, unsigned len);
int ZEXPORTVA c4_gzprintf (gzFile file, const char *format, /* args */ ...);
int ZEXPORT c4_gzputc(gzFile file, int c);
int ZEXPORT c4_gzputs(gzFile file, const char* s);
int ZEXPORT c4_gzflush (gzFile file, int flush);
z_off_t ZEXPORT c4_gzseek (gzFile file, z_off_t offset, int whence);
int ZEXPORT c4_gzrewind (gzFile file);
z_off_t ZEXPORT c4_gztell (gzFile file);
int ZEXPORT c4_gzeof (gzFile file);
int ZEXPORT c4_gzdirect (gzFile file);
int ZEXPORT c4_gzclose (gzFile file);
void ZEXPORT c4_gzclearerr (gzFile file);

}

#endif // C4_GZIO_H
