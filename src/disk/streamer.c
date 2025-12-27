#include "streamer.h"
#include "../memory/heap/kernel_heap.h"
#include "../status.h"
#include "../config.h"

struct disk_streamer* create_disk_streamer(int disk_id)
{
    disk_t *disk = get_disk(disk_id);
    if (disk == NULL)
    {
        return NULL;
    }
    disk_streamer_t *streamer = kernel_zero_alloc(sizeof(disk_streamer_t));
    if (streamer == NULL)
    {
        return NULL;
    }
    streamer->disk = disk;
    streamer->pos = 0;
    return streamer;
}

int disk_stream_seek(struct disk_streamer* streamer, uint32_t position)
{
    if (streamer == NULL)
    {
        return -MYOS_INVALID_ARG;
    }
    streamer->pos = position;
    return 1;
}

/*
    streamer : 디스크 스트리머 객체
    out      : 데이터를 읽어올 버퍼
    total    : 읽어올 총 바이트 수
    return   : 읽은 바이트 수 또는 오류 코드
*/

int disk_stream_read(struct disk_streamer* streamer, void* out, int total)
{
    if (streamer == NULL || total <= 0)
    {
        return -MYOS_INVALID_ARG;
    }

    int bytes_read = 0;
    unsigned char buffer[MYOS_SECTOR_SIZE];
    int sector_size = streamer->disk->sector_size;

    while (total > 0)
    {
        int sector_index = streamer->pos / sector_size;
        int offset = streamer->pos % sector_size;
        int to_read = total;
if (to_read > (sector_size - offset))
{
    to_read = sector_size - offset;
}

        int res = disk_read_block(streamer->disk, sector_index, 1, buffer);
        if (res < 0)
        {
            return res;
        }

        for (int i = 0; i < to_read; i++)
        {
            ((unsigned char*)out)[bytes_read + i] = buffer[offset + i];
        }

        streamer->pos += to_read;
        bytes_read += to_read;
        total -= to_read;
    }

    return bytes_read;
}

int disk_stream_write(struct disk_streamer* streamer, void *in, int total)
{
    if (streamer == NULL || total <= 0)
    {
        return -MYOS_INVALID_ARG;
    }

    int bytes_written = 0;
    unsigned char buffer[MYOS_SECTOR_SIZE];
    int sector_size = streamer->disk->sector_size;

    while (total > 0)
    {
        int sector_index = streamer->pos / sector_size;
        int offset = streamer->pos % sector_size;
        
        int to_write = total;
        if (to_write > (sector_size - offset))
        {
            to_write = sector_size - offset;
        }

        int res = disk_read_block(streamer->disk, sector_index, 1, buffer);
        if (res < 0)
        {
            return res;
        }

        for (int i = 0; i < to_write; i++)
        {
            buffer[offset + i] = ((unsigned char*)in)[bytes_written + i];
        }

        res = disk_write_block(streamer->disk, sector_index, 1, buffer);
        if (res < 0)
        {
            return res;
        }

        streamer->pos += to_write;
        bytes_written += to_write;
        total -= to_write;
    }

    return bytes_written;
}

void destroy_disk_streamer(struct disk_streamer* streamer)
{
    if (streamer)
    {
        kernel_free(streamer);
    }
}