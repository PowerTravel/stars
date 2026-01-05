#ifndef LIST_MACROS
#define LIST_MACROS
#define ListEnd(Sentinel, Element) ((Element) == (Sentinel))
#define ListEmpty(Sentinel) ((Sentinel)->Next == (Sentinel))

#define ListInitiate( Sentinel )               \
{                                              \
  (Sentinel)->Previous = (Sentinel);           \
  (Sentinel)->Next = (Sentinel);               \
}

#define ListInsertAfter( Sentinel, Element ) \
{                                            \
  (Element)->Previous = (Sentinel);          \
  (Element)->Next = (Sentinel)->Next;        \
  (Element)->Previous->Next = (Element);     \
  (Element)->Next->Previous = (Element);     \
}

#define ListInsertBefore( Sentinel, Element ) \
{                                             \
  (Element)->Previous = (Sentinel)->Previous; \
  (Element)->Next = (Sentinel);               \
  (Element)->Previous->Next = (Element);      \
  (Element)->Next->Previous = (Element);      \
}

#define ListRemove( Element )                      \
{                                                  \
  (Element)->Previous->Next = (Element)->Next;     \
  (Element)->Next->Previous = (Element)->Previous; \
}

#define ListCount( Sentinel, Type, Counter )  \
{                                             \
  Counter = 0;                                \
  for(Type* Element = (Sentinel)->Next;       \
    Element != (Sentinel);                    \
    Element = Element->Next)                  \
  {                                           \
    Counter++;                                \
  }                                           \
}


#define ListAdvanceBytePointer(Pointer, ByteCount) ((uint8_t*)Pointer) + (ByteCount);
#define ListRetreatBytePointer(Pointer, ByteCount) ((uint8_t*)Pointer) - (ByteCount);
#define ListAdvanceByType(Pointer, Type) ListAdvanceBytePointer(Pointer, sizeof(Type));
#define ListRetreatByType(Pointer, Type) ListRetreatBytePointer(Pointer, sizeof(Type));


#endif // LIST_MACROS

#ifndef TREE_MACROS
#define TREE_MACROS
#define TreeNodeInitiate( Sentinel )         \
{                                            \
  (Sentinel)->PreviousSibling = (Sentinel);  \
  (Sentinel)->NextSibling = (Sentinel);      \
}

#define TreeNodeInsertAfter( Sentinel, Element )       \
{                                                      \
  (Element)->PreviousSibling = (Sentinel);             \
  (Element)->NextSibling = (Sentinel)->NextSibling;    \
  (Element)->PreviousSibling->NextSibling = (Element); \
  (Element)->NextSibling->PreviousSibling = (Element); \
}

#define TreeNodeInsertBefore( Sentinel, Element )           \
{                                                           \
  (Element)->PreviousSibling = (Sentinel)->PreviousSibling; \
  (Element)->NextSibling = (Sentinel);                      \
  (Element)->PreviousSibling->NextSibling = (Element);      \
  (Element)->NextSibling->PreviousSibling = (Element);      \
}

#define TreeNodeInsertChild( Parent, Child )            \
{                                                       \
  if((Parent)->FirstChild)                              \
  {                                                     \
    (Child)->Parent = (Parent);                         \
    TreeNodeInsertBefore((Parent)->FirstChild, Child);  \
  }else{                                                \
    TreeNodeInitiate(Child);                            \
    (Child)->Parent = (Parent);                         \
    (Child)->Parent->FirstChild = Child;                \
  }                                                     \
}

#define TreeNodeRemoveSibling( Element )                                \
{                                                                       \
  (Element)->PreviousSibling->NextSibling = (Element)->NextSibling;     \
  (Element)->NextSibling->PreviousSibling = (Element)->PreviousSibling; \
}

#define TreeNodeRemoveParent( Child )                      \
{                                                          \
  if((Child)->Parent->FirstChild == (Child))               \
  {                                                        \
    if((Child)->NextSibling != (Child)){                   \
      (Child)->Parent->FirstChild = (Child)->NextSibling;  \
    }else{                                                 \
      (Child)->Parent->FirstChild = 0;                     \
    }                                                      \
  }                                                        \
}

#define TreeNodeRemove( Child )                      \
{                                                    \
  TreeNodeRemoveParent(Child);                       \
  TreeNodeRemoveSibling(Child);                      \
}

#endif // TREE_MACROS

// Randoms

#ifndef ArrayCount
#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]))
#endif // ArrayCount

#if JWIN_SLOW
  #ifndef Assert
  #define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
  #endif
#else
  #ifndef Assert
  #define Assert(Expression)
  #endif
#endif // JWIN_SLOW