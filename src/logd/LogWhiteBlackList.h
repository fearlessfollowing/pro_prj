#ifndef _LOGD_LOG_WHITE_BLACK_LIST_H__
#define _LOGD_LOG_WHITE_BLACK_LIST_H__

#include <sys/types.h>

#include <utils/List.h>

#include "LogBufferElement.h"

// White and Blacklist

class Prune {
    friend class PruneList;

    const uid_t mUid;
    const pid_t mPid;
    int cmp(uid_t uid, pid_t pid) const;

public:
    static const uid_t uid_all = (uid_t) -1;
    static const pid_t pid_all = (pid_t) -1;

    Prune(uid_t uid, pid_t pid);

    uid_t getUid() const { return mUid; }
    pid_t getPid() const { return mPid; }

    int cmp(LogBufferElement *e) const { return cmp(e->getUid(), e->getPid()); }

    // *strp is malloc'd, use free to release
    void format(char **strp);
};

typedef android::List<Prune *> PruneCollection;

class PruneList {
    PruneCollection mNaughty;
    PruneCollection mNice;
    bool mWorstUidEnabled;

public:
    PruneList();
    ~PruneList();

    int init(char *str);

    bool naughty(LogBufferElement *element);
    bool nice(LogBufferElement *element);
    bool worstUidEnabled() const { return mWorstUidEnabled; }

    // *strp is malloc'd, use free to release
    void format(char **strp);
};

#endif // _LOGD_LOG_WHITE_BLACK_LIST_H__
