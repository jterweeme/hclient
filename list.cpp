#include "list.h"

CList::CList()
{
    _list = &_lijst;
}

void CList::init()
{
    _lijst.Flink = _lijst.Blink = &_lijst;
}

PLIST_ENTRY CList::head()
{
    return _lijst.Flink;
}

PLIST_ENTRY CList::prev()
{
    return _lijst.Blink;
}

PLIST_ENTRY CList::get()
{
    return &_lijst;
}

void CList::insertTail(LIST_ENTRY *node)
{
    LIST_ENTRY *xList = &_lijst;
    LIST_ENTRY *xBlink = xList->Blink;
    node->Flink = xList;
    node->Blink = xBlink;
    xBlink->Flink = node;
    xList->Blink = node;
}

BOOL CList::isEmpty() const
{
    return _lijst.Flink == &_lijst;
}

PLIST_ENTRY CList::removeHead()
{
    LIST_ENTRY *ret = head();
    remove(_list->Flink);
    return ret;
}

#if 0
void CList::destroyWithCallback(PLIST_CALLBACK *cb)
{
    LIST_ENTRY *currNode;

    while (!isEmpty())
    {
        currNode = removeHead();
        cb(currNode);
    }
}
#endif

void CList::remove(LIST_ENTRY *node)
{
    LIST_ENTRY *xFlink = node->Flink;
    LIST_ENTRY *xBlink = node->Blink;
    xBlink->Flink = xFlink;
    xFlink->Blink = xBlink;
}

CListIterator::CListIterator(CList *list) : _list(list)
{
    _current = _list->head();
}

BOOL CListIterator::hasNext()
{
    return _current != _list->get();
}

PLIST_ENTRY CListIterator::next()
{
    LIST_ENTRY *ret = _current;
    _current = _current->Flink;
    return ret;
}

