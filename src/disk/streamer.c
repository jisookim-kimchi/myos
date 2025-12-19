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

int disk_stream_read(struct disk_streamer* streamer,void*out, int total)
{
    if (streamer == NULL)
    {
        return -MYOS_INVALID_ARG;
    }
    int sector_index = streamer->pos / streamer->disk->sector_size; //100 / 512 = 0
    int offset = streamer->pos % streamer->disk->sector_size; //100 % 512 = 100
    unsigned char buffer[MYOS_SECTOR_SIZE];

    // 먼저 섹터 전체를 읽음
    int res = disk_read_block(streamer->disk, sector_index, 1, buffer);
    if (res < 0)
    {
        goto out;
    }

    int sector_size = streamer->disk->sector_size;
    int to_read = total > sector_size ? sector_size : total;
    for (int i = 0; i < to_read; i++)
    {
        ((unsigned char*)out)[i] = buffer[offset + i];
    }

    // update streamer status
    streamer->pos += to_read;
    if (total > sector_size)
    {
        // write the next chunk after the data we've already written
        res = disk_stream_read(streamer, (unsigned char*)out + to_read, total - to_read);
        if (res >= 0)
        {
            res += to_read; // accumulate bytes read
        }
    }
    else
    {
        res = to_read;
    }

out:
    return res;
}

int disk_stream_write(struct disk_streamer* streamer, void* in, int total)
{
    if (streamer == NULL)
    {
        return -MYOS_INVALID_ARG;
    }
    int sector_index = streamer->pos / streamer->disk->sector_size;
    int offset = streamer->pos % streamer->disk->sector_size;
    unsigned char buffer[MYOS_SECTOR_SIZE];

    int res = disk_read_block(streamer->disk, sector_index, 1, buffer);
    if (res < 0)
    {
        goto out;
    }

    int sector_size = streamer->disk->sector_size;
    int to_write = total;
    if (total > (sector_size - offset))
    {
        to_write = sector_size - offset;
    }
    
    for (int i = 0; i < to_write; i++)
    {
        buffer[offset + i] = ((unsigned char*)in)[i];
    }

    res = disk_write_block(streamer->disk, sector_index, 1, buffer);
    if (res < 0)
    {
        goto out;
    }

    streamer->pos += to_write;
    if (total > to_write)
    {
        res = disk_stream_write(streamer, (unsigned char*)in + to_write, total - to_write);
        if (res >= 0)
        {
            res += to_write;
        }
    }
    else
    {
        res = to_write;
    }

out:
    return res;
}

void destroy_disk_streamer(struct disk_streamer* streamer)
{
    if (streamer)
    {
        kernel_free(streamer);
    }
}