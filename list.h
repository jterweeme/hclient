#ifndef LIST_H
#define LIST_H

#include <windows.h>

typedef VOID PLIST_CALLBACK(PLIST_ENTRY);

class CList
{
private:
    LIST_ENTRY _lijst;
    LIST_ENTRY *_list;
public:
    CList();
    void init();
    PLIST_ENTRY get();
    PLIST_ENTRY head();
    PLIST_ENTRY prev();
    void remove(LIST_ENTRY *node);
    PLIST_ENTRY removeHead();
    void insertTail(LIST_ENTRY *node);
    BOOL isEmpty() const;
    void destroyWithCallback(PLIST_CALLBACK cb);
};

class CListIterator
{
private:
    CList *_list;
    LIST_ENTRY *_current;
public:
    CListIterator(CList *list);
    BOOL hasNext();
    PLIST_ENTRY next();
};

#endif
