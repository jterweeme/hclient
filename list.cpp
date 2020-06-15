#include "list.h"

CList::CList(LIST_ENTRY *list) : _list(list)
{

}

void CList::init()
{
    _list->Flink = _list->Blink = _list;
}

PLIST_ENTRY CList::next()
{
    return _list->Flink;
}

PLIST_ENTRY CList::head()
{
    return _list->Flink;
}

PLIST_ENTRY CList::prev()
{
    return _list->Blink;
}

void CList::insertTail(LIST_ENTRY *node)
{
    LIST_ENTRY *xList = _list;
    LIST_ENTRY *xBlink = xList->Blink;
    node->Flink = xList;
    node->Blink = xBlink;
    xBlink->Flink = node;
    xList->Blink = node;
}

BOOL CList::isEmpty() const
{
    return _list->Flink == _list;
}

static BOOL IsListEmpty(LIST_ENTRY *list)
{
    return list->Flink == list;
}

static PLIST_ENTRY GetListHead(LIST_ENTRY *list)
{
    return list->Flink;
}

static PLIST_ENTRY RemoveHead(PLIST List)
{
    PLIST_NODE_HDR ret = GetListHead(List);
    RemoveNode(List->Flink);
    return ret;
}

VOID DestroyListWithCallback(LIST_ENTRY *list, PLIST_CALLBACK cb)
{
    LIST_NODE_HDR *currNode;

    while (!IsListEmpty(list))
    {
        currNode = RemoveHead(list);
        cb(currNode);
    }
}

VOID RemoveNode(LIST_ENTRY *node)
{
    LIST_ENTRY *xFlink = node->Flink;
    LIST_ENTRY *xBlink = node->Blink;
    xBlink->Flink = xFlink;
    xFlink->Blink = xBlink;
}

