#ifndef LIST_H
#define LIST_H

#include <windows.h>

typedef LIST_ENTRY LIST_NODE_HDR, *PLIST_NODE_HDR;
typedef LIST_NODE_HDR LIST, *PLIST;
typedef VOID PLIST_CALLBACK(PLIST_NODE_HDR);
VOID RemoveNode(LIST_ENTRY *ListNode);
VOID DestroyListWithCallback(LIST_ENTRY *List, PLIST_CALLBACK Callback);

class CList
{
private:
    LIST_ENTRY *_list;
public:
    CList(LIST_ENTRY *list);
    void init();
    PLIST_ENTRY next();
    PLIST_ENTRY head();
    PLIST_ENTRY prev();
    void insertTail(LIST_ENTRY *node);
    BOOL isEmpty() const;
};
    
#endif
