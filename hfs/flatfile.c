#include <stdlib.h>
#include <fcntl.h>
#include "hfsplus.h"

#define BLOCKSIZE 8192

#define min(a, b) ((a) < (b) ? (a) : (b))

struct ffinfo {
  int fd;
  off_t last_bloc;
  char dirty;
  char scratch[BLOCKSIZE];
};
  
static int flatFileRW(io_func* io, off_t location, size_t size, void *buffer, int iswrite) {
  struct ffinfo *info = (void *) io->data;
  char *buf = buffer;
  
  if(size == 0) {
    return TRUE;
  }

  while(size) {
    off_t bloc = location & ~(BLOCKSIZE - 1);

    if(info->last_bloc != bloc) {
      if(info->dirty) {
        if(pwrite(info->fd, info->scratch, BLOCKSIZE, info->last_bloc) != BLOCKSIZE) {
          perror("pwrite");
          return FALSE;
        }
      }
      if(pread(info->fd, info->scratch, BLOCKSIZE, bloc) != BLOCKSIZE) {
        perror("pread");
        return FALSE;
      }
      info->last_bloc = bloc;
    }
    
    int s = (location & (BLOCKSIZE - 1));
    int z = min(size, BLOCKSIZE - s);
    //printf("s=%d z=%d w=%d\n", s, z, (int)iswrite);
    if(iswrite) {
      memcpy(info->scratch + s, buf, z);
      info->dirty = 1;
    } else {
      memcpy(buf, info->scratch + s, z);
    }
    
    location += z;
    size -= z;
    buf += z;
  }
  
  return TRUE;  
}

static int flatFileRead(io_func* io, off_t location, size_t size, void *buffer) {
  return flatFileRW(io, location, size, buffer, 0);
}

static int flatFileWrite(io_func* io, off_t location, size_t size, void *buffer) {
  return flatFileRW(io, location, size, buffer, 1);
}

static void closeFlatFile(io_func* io) {
  int fd;
  struct ffinfo *info = (void *) io->data;
  if(info->dirty && info->last_bloc != (off_t) -1) {
    if(pwrite(info->fd, info->scratch, BLOCKSIZE, info->last_bloc) != BLOCKSIZE) {
      perror("pwrite");
    }
  }
  
  close(info->fd);
  free(info);
  free(io);
}

io_func* openFlatFile(const char* fileName) {
  io_func* io;
  
  io = (io_func*) malloc(sizeof(io_func));
  
  int fd = open(fileName, O_RDWR);
  
  if(fd == -1) {
    perror("open");
    return NULL;
  }

  struct ffinfo *info = malloc(sizeof(struct ffinfo));
  info->fd = fd;
  info->last_bloc = -1;
  info->dirty = 0;
  io->data = info;
  
  io->read = &flatFileRead;
  io->write = &flatFileWrite;
  io->close = &closeFlatFile;
  
  return io;
}

// vim: tabstop=2:softtabstop=2:shiftwidth=2:expandtab
