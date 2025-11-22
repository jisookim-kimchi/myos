#include "streamer.h"

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

    int to_read = total > MYOS_SECTOR_SIZE ? MYOS_SECTOR_SIZE : total;
    for (int i = 0; i < to_read; i++)
    {
        ((unsigned char*)out)[i] = buffer[offset + i];
    }

    //update stremaer status
    streamer->pos += to_read;
    if (total > MYOS_SECTOR_SIZE)
    {
        res = disk_stream_read(streamer, (unsigned char*)out, total - MYOS_SECTOR_SIZE);
    }
    else
    {
        res = to_read;
    }

out:
    return res;
}

void disk_streamer_close(struct disk_streamer *streamer)
{
    return ;
}

void destroy_disk_streamer(struct disk_streamer* streamer)
{
    if (streamer)
    {
        kernel_free(streamer);
    }
}