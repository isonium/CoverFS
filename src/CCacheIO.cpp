#include "CCacheIO.h"

// -----------------------------------------------------------------

CBlock::CBlock(CAbstractBlockIO &_bio, CEncrypt &_enc, int _blockidx, int8_t *_buf) : dirty(false), blockidx(_blockidx), bio(_bio), enc(_enc), buf(_buf) 
{
}

void CBlock::Dirty()
{
    // TODO: assert if mutex is locked. Cannot do, because std::mutex doesn't provide such a function 
    dirty = true;
}

// TODO: maybe provide an extra class with special read and write access [] and lock_guards and so on.

int8_t* CBlock::GetBuf()
{
    mutex.lock();
    enc.Decrypt(blockidx, buf);
    return buf;
}

void CBlock::ReleaseBuf()
{
    enc.Encrypt(blockidx, buf);
    mutex.unlock();
}

// -----------------------------------------------------------------

CCacheIO::CCacheIO(CAbstractBlockIO &_bio, CEncrypt &_enc) : bio(_bio), enc(_enc)
{
    blocksize = bio.blocksize;
}

CBLOCKPTR CCacheIO::GetBlock(const int blockidx)
{
    auto cacheblock = cache.find(blockidx);
    if (cacheblock != cache.end())
    {
        return cacheblock->second;
    }
    int8_t *buf = new int8_t[blocksize];
    bio.Read(blockidx, buf);
    
    CBLOCKPTR block(new CBlock(bio, enc, blockidx, buf));
    cache[blockidx] = block;
    return block;
}

size_t CCacheIO::GetFilesize()
{
    return bio.GetFilesize();
}

void CCacheIO::Sync()
{
    for(auto it = cache.begin(); it != cache.end(); ++it) 
    {
        CBLOCKPTR block = it->second;

        if (block->dirty)
        {
        if (block->mutex.try_lock())
        {
            //printf("Sync block %i\n", block->blockidx);                        
            bio.Write(block->blockidx, block->buf);
            block->mutex.unlock();
            block->dirty = false;
        } /*else
        {
             fprintf(stderr, "Sync of locked data\n");
             exit(1);
        }*/
        }
    }
}
