/* 
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Modified to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
    queue_t *q =  malloc(sizeof(queue_t));
    if (q == NULL)
        return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    list_ele_t *lp, *nxt;
    if (q == NULL)
        return;
    for (lp = q->head; lp != NULL; lp = nxt) {
        nxt = lp->next;
        if (lp->value != NULL)
            free(lp->value);
        free(lp);
    }
    free(q);
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh;
    if (q == NULL)
        return false;
    if ((newh = malloc(sizeof(list_ele_t))) == NULL)
        return false;
    if ((newh->value = malloc(strlen(s) + 1)) == NULL) {
        free(newh);
        return false;
    }
    strcpy(newh->value, s);
    newh->next = q->head;
    if (q->tail == NULL) /* empty quene */
        q->tail = newh;
    q->head = newh;
    q->size++;
    return true;
}

/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    list_ele_t *newt;
    if (q == NULL)
        return false;
    if ((newt = malloc(sizeof(list_ele_t))) == NULL)
        return false;
    if ((newt->value = malloc(strlen(s) + 1)) == NULL) {
        free(newt);
        return false;
    }
    strcpy(newt->value, s);
    newt->next = NULL;
    if (q->head == NULL) /* empty quene */
        q->head = newt;
    else /* not empty */
        q->tail->next = newt;
    q->tail = newt;
    q->size++;
    return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to *sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    list_ele_t *remove;
    if (q == NULL || (remove = q->head) == NULL)
        return false;
    if (sp != NULL && remove->value != NULL) {
        char *str = remove->value;
        while (--bufsize > 0 && *str != '\0')
            *sp++ = *str++;
        *sp = '\0';
    }
    if (remove->value != NULL)
        free(remove->value);
    if ((q->head = remove->next) == NULL)
        q->tail = NULL;
    free(remove);
    q->size--;
    return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    if (q == NULL)
        return 0;
    return q->size;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    list_ele_t *lp, *nxt, *prv;
    if (q != NULL) {
        for (lp = q->head, prv = NULL; lp != NULL; lp = nxt) {
            nxt = lp->next;
            lp->next = prv;
            prv = lp;
        }
        q->tail = q->head;
        q->head = prv;
    }
}
