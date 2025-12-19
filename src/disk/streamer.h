#ifndef STREAMER_H
#define STREAMER_H

#include <stdint.h>
#include "disk.h"


typedef struct disk_streamer
{
    disk_t *disk;         // 대상 디스크
    uint32_t pos;         // 현재 스트림 위치 (바이트 단위)
} disk_streamer_t;

struct disk_streamer* create_disk_streamer(int disk_id);
int     disk_stream_seek(struct disk_streamer* streamer, uint32_t position);
int     disk_stream_read(struct disk_streamer* streamer,void*out, int total);
int     disk_stream_write(struct disk_streamer* streamer, void* in, int total);
void    destroy_disk_streamer(struct  disk_streamer* streamer);
#endif