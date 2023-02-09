#ifndef RPMSG_DUMMY_LIB_TI_RPMSG_CHAR_H
#define RPMSG_DUMMY_LIB_TI_RPMSG_CHAR_H

#include "rproc_id.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define RPMSG_DEV_NAME_MAXLEN 32

    struct rpmsg_char_dev
    {
        int fd;
        int endpt;
    };

    typedef struct rpmsg_char_dev rpmsg_char_dev_t;

    // FIXME (aw): we should create a real file descriptor, which behaves correct

    inline rpmsg_char_dev_t *rpmsg_char_open(enum rproc_id id, char *dev_name,
                                             int remote_endpt, char *eptdev_name,
                                             int flags)
    {
        return nullptr;
    }

    inline int rpmsg_char_close(rpmsg_char_dev_t *rcdev)
    {
        return 0;
    }

    inline int rpmsg_char_init(char *soc_name)
    {
        return 0;
    }

    inline void rpmsg_char_exit(void)
    {
    }

#if defined(__cplusplus)
}
#endif

#endif // RPMSG_DUMMY_LIB_TI_RPMSG_CHAR_H
