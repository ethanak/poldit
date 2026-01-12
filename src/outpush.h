#ifndef OUTPUSH_H
#define OUTPUSH_H



// wstawia string do bufora, poprzedza spacją jeśli pottrzebna

#define pushbufs(s) do {const char *sc=_naturalize(s,buffer->flags & FLG_NATURAL);int xs = strlen(sc); \
    if (!buffer->blank) { \
        if (buffer->outbuf) *buffer->outbuf++ = ' '; \
        len += 1; \
    } \
    if (buffer->outbuf) {strcpy(buffer->outbuf,sc);buffer->outbuf += xs;} \
    len += xs; buffer->blank = 0; \
} while(0)

// wstawia string fo bufora

#define pushbuf(s) do {int xs = strlen(s); \
    if (buffer->outbuf) {strcpy(buffer->outbuf,s);buffer->outbuf += xs;} \
    len += xs; buffer->blank = 0; \
} while(0)

// wstawia string określonej długości do bufora
// wstawia spację przed stringiem jeśli potrzebna
// ustawia marker spacji w buforze na podstawie ostatniego znaku

#define pushbufl(s, xs) do { \
    if (!buffer->blank) { \
        if (_needBlank(s)) { \
            if (buffer->outbuf) *buffer->outbuf++=' '; \
            len += 1; \
        } \
    } \
    if (buffer->outbuf) { \
        strncpy(buffer->outbuf,s,xs); \
        (buffer->outbuf) += xs; \
    } \
    buffer->blank = _lastBlank(s, xs); \
    len += xs; \
} while (0)

#endif
