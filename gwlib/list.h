/*
 * list.h - generic dynamic list
 *
 * This is a generic, dynamic list. Generic means that it stores void pointers
 * (void *), instead of a more specific data type. This allows storage of
 * any type of items in the list. The pointers may not be NULL pointers.
 *
 * A number of operations are defined for the list: create, destroy,
 * query of length, inserting and deleting items, getting items, and so on.
 * See below for a detailed list.
 *
 * The list is also thread safe: each single operation is atomic. For
 * list manipulation that needs to be atomic but uses several single
 * operations, the list supports locking and unlocking. It is up to the
 * caller to make sure the list lock is used properly; the implementation
 * only guarantees the atomicity of single operations.
 *
 * The API also has functions for solving typical producer-consumer problems:
 * the list counts the number of producers it has (they need to register
 * _and_ unregister explicitly) and has functions for adding a produced
 * item to the list and removing an item so that it can be consumed. The
 * consumption function (`list_consume') sleeps, without using processor
 * time, until there is an item to be consumed or there are no more
 * producers. Thus, a typical producer would look like this:
 *
 *	list_add_producer(list);
 *	while ((item = foo()) != NULL)
 *		list_produce(list, item);
 *	list_remove_producer(list);
 *
 * and the corresponding consumer would look like this:
 *
 *	while ((item = list_consume(list)) != NULL)
 *		bar(item);
 *
 * There can be any number of producers and consumers at the same time.
 *
 * List items are numbered starting with `0'.
 *
 * Most list functions can do memory allocations. If these allocations
 * fail, they will kill the program (they use gwlib/gwmem.c for
 * memory allocations, and those do the killing). This is not mentioned
 * explicitly for each function.
 *
 * The module prefix is `list' (in any combination of upper and lower case
 * characters). All externally visible symbols (i.e., those defined by
 * this header file) start with the prefix.
 *
 * Lars Wirzenius <liw@wapit.com>
 */

#ifndef LIST_H
#define LIST_H


/*
 * The list type. It is opaque: do not touch it except via the functions
 * defined in this header.
 */
typedef struct List List;


/*
 * A comparison function for list items. Returns true (non-zero) for
 * equal, false for non-equal. Gets an item from the list as the first
 * argument, the pattern as a second argument.
 */
typedef int list_item_matches_t(void *item, void *pattern);


/*
 * Create a list and return a pointer to the list object.
 */
List *list_create(void);

/*
 * Destroy the list. Note that the caller is responsible for destroying
 * the items in the list and for making sure that nothing else touches
 * the list (in any way) while it is being destroyed. This function only
 * frees the memory associated with the list itself.
 */
void list_destroy(List *list);


/*
 * Return the number of items in the list.
 */
long list_len(List *list);


/*
 * Add a new item to the end of the list.
 */
void list_append(List *list, void *item);


/*
 * Insert an item into the list so that it becomes item number `pos'.
 */
void list_insert(List *list, long pos, void *item);


/*
 * Delete items from the list. Note that this does _not_ free the memory
 * for the items, they are just dropped from the list.
 */
void list_delete(List *list, long pos, long count);


/*
 * Delete all items from the list that match `pattern'. Like list_delete,
 * the items are removed from the list, but are not destroyed themselves.
 */
void list_delete_all(List *list, void *pat, list_item_matches_t *cmp);


/*
 * Delete all items from the list whose pointer value is exactly `item'.
 */
void list_delete_equal(List *list, void *item);


/*
 * Return the item at position `pos'.
 */
void *list_get(List *list, long pos);


/*
 * Remove and return the first item in the list. Return NULL if list is
 * empty. Note that unlike list_consume, this won't sleep until there is
 * something in the list.
 */
void *list_extract_first(List *list);


/*
 * Create a new list with items from `list' that match a pattern. The items
 * are removed from `list'. Return NULL if no matching items are found.
 * Note that unlike list_consume, this won't sleep until there is
 * something in the list.
 */
List *list_extract_all(List *list, void *pat, list_item_matches_t *cmp);


/*
 * Lock the list. This protects the list from other threads that also
 * lock the list with list_lock, but not from threads that do not.
 * (This is intentional.)
 */
void list_lock(List *list);


/*
 * Unlock the list lock locked by list_lock. Only the owner of the lock
 * may unlock it (although this might not be checked).
 */
void list_unlock(List *list);


/*
 * Sleep until the list is non-empty. Note that after the thread awakes
 * another thread may already have emptied the list again. Those who wish
 * to use this function need to be very careful with list_lock and
 * list_unlock.
 */
int list_wait_until_nonempty(List *list);


/*
 * Register a new producer to the list.
 */
void list_add_producer(List *list);

/*
 * Return the current number of producers for the list
 */
int list_producer_count(List *list);

/*
 * Remove a producer from the list. If the number of producers drops to
 * zero, all threads sleeping in list_consume will awake and return NULL.
 */
void list_remove_producer(List *list);


/*
 * Add an item to the list. This equivalent to list_append, but may be
 * easier to remember.
 */
void list_produce(List *list, void *item);


/*
 * Remove an item from the list, or return NULL if the list was empty
 * and there were no producers. If the list is empty but there are
 * producers, sleep until there is something to return.
 */
void *list_consume(List *list);


/*
 * Search the list for a particular item. If not found, return NULL. If found,
 * return the list element. Compare items to search pattern with 
 * `cmp(item, pattern)'. If the function returns non-zero, the items are equal.
 */
void *list_search(List *list, void *pattern, list_item_matches_t *cmp);


/*
 * Search the list for all items matching a pattern. If not found, return 
 * NULL. If found, return a list with the matching elements. Compare items
 * to search pattern with `cmp(item, pattern)'. If the function returns 
 * non-zero, the items are equal.
 */
List *list_search_all(List *list, void *pattern, list_item_matches_t *cmp);

/*
 * Put lists one after another. List2 will be destroyed in the process. Return
 * the united list.
 */
List *list_cat(List *list1, List *list2);

#endif
