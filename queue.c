#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *head =
        (struct list_head *) malloc(sizeof(struct list_head));
    if (!head)
        return NULL;
    INIT_LIST_HEAD(head);

    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (l == NULL)
        return;
    struct list_head *node = l->next;

    while (!list_empty(l)) {
        struct list_head *tmp = node;
        node = node->next;
        list_del(tmp);
        q_release_element(list_entry(tmp, element_t, list));
    }
    free(l);
}

/*
 * Bypass the memory leak error from Cppcheck.
 * This function allocates a element_t then return it's pointer.
 */
element_t *create_node(void)
{
    return (element_t *) malloc(sizeof(element_t));
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    /*
     * Consider the following two situation:
     *     (1) head == NULL or head->prev == NULL or head->next == NULL
     *     (2) head != NULL
     * For the (1) situation, just simply return false.
     * For the (2) situation, since the initialized list_head pointer's
     * prev was pointed to the list_head pointer itself, we can finish the
     * insertion via `list_add` API.
     */

    // If the queue is NULL or uninitialized
    if (!head || !(head->prev) || !(head->next))
        return false;

    // Create a node
    // element_t *node = (element_t *) malloc(1 * sizeof(element_t));
    element_t *node = create_node();
    if (node == NULL)
        return false;
    // Calculate the length of s then copy it into the node
    // Maximum length of the string is 1024 (excluding '\0')
    size_t len_s = strnlen(s, 1024) + 1;
    node->value = (char *) malloc(len_s * sizeof(char));
    if (node->value == NULL) {
        free(node);
        return false;
    }
    strncpy(node->value, s, len_s);

    // Linux Kernel style API, create a struct list_head variable
    LIST_HEAD(list);
    node->list = list;
    // Linux Kernel style API, add the node before the head
    list_add(&(node->list), head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    // If the queue is NULL or uninitialized
    if (!head || !(head->prev) || !(head->next))
        return false;

    // Create a node
    // element_t *node = (element_t *) malloc(1 * sizeof(element_t));
    element_t *node = create_node();
    if (node == NULL)
        return false;
    // Calculate the length of s then copy it into the node
    // Maximum length of the string is 1024 (excluding '\0')
    size_t len_s = strnlen(s, 1024) + 1;
    node->value = (char *) malloc(len_s * sizeof(char));
    if (node->value == NULL) {
        free(node);
        return false;
    }
    strncpy(node->value, s, len_s);

    // Linux Kernel style API, create a struct list_head variable
    LIST_HEAD(list);
    node->list = list;
    // Linux Kernel style API, append the node at the tail of the queue
    list_add_tail(&node->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->next == head)
        return NULL;
    element_t *ret_p = list_entry(head->next, element_t, list);

    list_del(head->next);
    if (sp) {
        strncpy(sp, ret_p->value, bufsize);
        // Credit to @laneser's note
        sp[bufsize - 1] = '\0';
    }
    return ret_p;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->next == head)
        return NULL;
    element_t *ret_p = list_entry(head->prev, element_t, list);

    list_del(head->prev);
    if (sp) {
        strncpy(sp, ret_p->value, bufsize);
        // Credit to @laneser's note
        sp[bufsize - 1] = '\0';
    }
    return ret_p;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *node;

    list_for_each (node, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;

    struct list_head **indirect = &(head->next);
    for (struct list_head *fast = head->next;
         fast != head && fast->next != head; fast = fast->next->next) {
        indirect = &(*indirect)->next;
    }
    struct list_head *tmp = *indirect;
    list_del(tmp);
    q_release_element(list_entry(tmp, element_t, list));

    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    /* This implementation will gather all duplicate strings then
     * delete them together.
     */
    if (!head || head->next == head)
        return NULL;
    struct list_head *dup_strs = q_new();
    struct list_head *node, *safe;
    char *val = list_entry(head->next, element_t, list)->value;
    for (node = head->next->next, safe = node->next; node != head;
         node = safe, safe = node->next) {
        char *tmp = list_entry(head->next, element_t, list)->value;
        if (strcmp(val, tmp) == 0)
            list_move(node, dup_strs);
        else
            val = tmp;
    }
    q_free(dup_strs);
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;
    struct list_head *node;
    for (node = head->next; node != head && node->next != head;
         node = node->next) {
        struct list_head *tmp = node->next;
        list_move(node, tmp);
    }
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || head->next == head)
        return;
    struct list_head *node = head;
    // Swap all node's next and prev pointer
    do {
        struct list_head *tmp = node->next;
        node->next = node->prev;
        node->prev = tmp;

        node = node->prev;
    } while (node != head);
}

/*
 * Merge two sorted list to a sorted one
 * Must ensure the left list isn't empty
 */
void merge_two_list(struct list_head *left_head, struct list_head *right_head)
{
    struct list_head *safe, *right, *left = left_head->next;
    // if the left list is empty but the right isn't
    if (left == left_head && right_head->next != right_head) {
        // left_head = right_head;
        list_splice(right_head, left_head);
        return;
    }
    // Put right list's node to left
    list_for_each_safe (right, safe, right_head) {
        element_t *l_node = list_entry(left, element_t, list);
        element_t *r_node = list_entry(right, element_t, list);

        int cmp_result = strcmp(l_node->value, r_node->value);
        // if left value <= right value, move the left pointer to it's next
        while (left->next != left_head && cmp_result <= 0) {
            left = left->next;
            l_node = list_entry(left, element_t, list);
            cmp_result = strcmp(l_node->value, r_node->value);
        }
        list_del(right);
        if (cmp_result > 0)
            list_add_tail(right, left);
        else
            list_add(right, left);
    }
}
/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head || head->next == head || list_is_singular(head))
        return;
    struct list_head *slow = head->next, *fast = head->next->next;
    while (fast != head && fast->next != head) {
        slow = slow->next;
        fast = fast->next->next;
    }
    // Now the slow pointer is at the middle
    LIST_HEAD(right);

    // Split the list from the next node of slow pointer
    right.next = slow->next;
    right.next->prev = &right;
    right.prev = head->prev;
    right.prev->next = &right;

    slow->next = head;
    head->prev = slow;

    q_sort(&right);
    q_sort(head);

    merge_two_list(head, &right);
}
