#ifndef _WS_C_LOCK_MUTEX_H_
#define _WS_C_LOCK_MUTEX_H_
#include "SDL2/SDL_mutex.h"


class CLock 
{
public:
    CLock(SDL_mutex *pMutex) 
    {
        m_pMutex = pMutex;
        SDL_LockMutex(m_pMutex);
    }

    ~CLock()
    {
        SDL_UnlockMutex(m_pMutex);
    }

private:
    SDL_mutex *m_pMutex;
};

#endif // _WS_C_LOCK_MUTEX_H_