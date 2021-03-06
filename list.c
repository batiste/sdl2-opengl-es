#include <stdlib.h>

void assert(int value) {
    if(value)
        return;
    printf("assert fail\n");
    exit(1);
}

//Generic list Element
struct ListElement {
    struct ListElement *next;
    struct ListElement *prev;
    void *data;
};
typedef struct ListElement ListElement;

//Generic List Structure
struct GenericList {
    int length;      //Number of elements in list
    struct ListElement *first;  //Ptr to first element in list
    struct ListElement *last;   //Ptr to last element in list
};
typedef struct GenericList GenericList;

void createList(struct GenericList *list) {
    list->length = 0;
    list->first = NULL;
    list->last = NULL;
}

ListElement *addToList(GenericList *list, void *item) {
    //check inputs
    assert(item!=NULL);
    assert(list!=NULL);
    //Create generic element to hold item ptr
    ListElement *newElement;
    newElement = (ListElement *)malloc(sizeof(newElement));  //create generic element
    assert(newElement != NULL);
    list->length = list->length + 1;
    newElement->data = item;
    if (list->length == 1)
    {
        list->last = newElement;
        newElement->prev = NULL;
        newElement->next = NULL;
    }
    else
    {
        newElement->prev = NULL;
        newElement->next = list->first;
        list->first->prev = newElement;
    }
    list->first = newElement;
    return newElement;
}

int removeFromList(GenericList *list, ListElement *toRemove) {
    // TODO: find a mechanisms to free deleted items

    //check inputs
    if(toRemove==NULL) {
        return 0;
    }
    assert(list!=NULL);
    ListElement *el;
    if (list->length == 0) {
        return 0;
    }

    if (list->length == 1) {
        if(toRemove == list->first) {
            list->length = 0;
            list->first = NULL;
            list->last = NULL;
            return 1;
        }
        return 0;
    }

    // there is at least 2 items in the list
    for(el = list->first; el != NULL; el=el->next) {
        // found a matching item
        if(el == toRemove) {
            // this is the first item
            if(el == list->first) {
                list->first = el->next;
                list->first->prev = NULL;
            // this is the last item
            } else if (el == list->last) {
                list->last = el->prev;
                list->last->next = NULL;
            } else {
                el->prev->next = el->next;
                el->next->prev = el->prev;
            }
            list->length = list->length - 1;
            return 1;
        }
    }
    return 0;
}


void displayList(GenericList *list) {
    ListElement *el;
    printf("List length %d\n", list->length);
    int i = 1;
    for(el = list->first; el != NULL; el=el->next, i=i+1) {
        printf("Item %d address %d\n", i, (int)el->data);
    }
}