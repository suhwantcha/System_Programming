#include "list.h"
#include "hash.h"
#include "bitmap.h"
#include "debug.h"
#include "hex_dump.h"
#include "limits.h"
#include "round.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONTAINERS 10

//struct list_item { struct list_elem elem; int data; };
struct hash_item { struct hash_elem elem; int data; };

struct list *lists[MAX_CONTAINERS];
struct hash *hashes[MAX_CONTAINERS];
struct bitmap *bitmaps[MAX_CONTAINERS];
char names[MAX_CONTAINERS][20];
int list_count = 0, hash_count = 0, bm_count = 0;

// Hash function for hash table using hash_int from library
// Hash table에서 정수를 저장하고 검색할 때 사용할 해시 값을 생성
unsigned my_hash_int(const struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, elem);
    ASSERT(item != NULL);
    return hash_int(item->data);
}

// Compares two hash elements for sorting
bool hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct hash_item *item_a = hash_entry(a, struct hash_item, elem);
    struct hash_item *item_b = hash_entry(b, struct hash_item, elem);
    ASSERT(item_a != NULL && item_b != NULL);
    return item_a->data < item_b->data;
}

// hash element의 data 제곱
void hash_square(struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, elem);
    item->data = item->data * item->data;
}

// hash element의 data 세제곱
void hash_triple(struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, elem);
    item->data = item->data * item->data * item->data;
}

// Finds the index of a container by name
int find_container(const char *name) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (strcmp(names[i], name) == 0) {
            if (i < list_count) return i;
            if (i < hash_count) return i;
            if (i < bm_count) return i;
        }
    }
    return -1;
}

// Gets the list element at a specific position
struct list_elem *get_elem_at(struct list *list, size_t pos) {
    struct list_elem *e = list_begin(list);
    for (size_t i = 0; i < pos && e != list_end(list); i++) {
        e = list_next(e);
    }
    return e;
}


int main() {
    char command[20], type[20], name[20], name2[20], bool_str[6], func[20];
    int value, size, start, cnt, pos1, pos2;
    bool bool_val;

    while (1) {
        scanf("%s", command);
        if (strcmp(command, "quit") == 0) break;

        // Creates a new list, hash table, or bitmap
        if (strcmp(command, "create") == 0) {
            scanf("%s %s", type, name);
            int idx = find_container(name);
            if (idx != -1) continue;

            if (strcmp(type, "list") == 0) {
                lists[list_count] = malloc(sizeof(struct list));
                ASSERT(lists[list_count] != NULL);
                list_init(lists[list_count]);
                strcpy(names[list_count], name);
                list_count++;
            }
            else if (strcmp(type, "hashtable") == 0) {
                hashes[hash_count] = malloc(sizeof(struct hash));
                ASSERT(hashes[hash_count] != NULL);
                hash_init(hashes[hash_count], my_hash_int, hash_less, NULL);
                strcpy(names[hash_count], name);
                hash_count++;
            }
            else if (strcmp(type, "bitmap") == 0) {
                scanf("%d", &size);
                ASSERT(size > 0 && size <= SIZE_MAX);
                bitmaps[bm_count] = bitmap_create(size);
                ASSERT(bitmaps[bm_count] != NULL);
                strcpy(names[bm_count], name);
                bm_count++;
            }
            else {
                PANIC("Invalid type");
            }
        }
            // Prints the contents of a container
        else if (strcmp(command, "dumpdata") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx == -1) continue;

            if (idx < list_count) {
                struct list_elem *e;
                for (e = list_begin(lists[idx]); e != list_end(lists[idx]); e = list_next(e)) {
                    struct list_item *item = list_entry(e, struct list_item, elem);
                    printf("%d ", item->data);
                }
                printf("\n");
            }
            else if (idx < hash_count) {
                struct hash_iterator it;
                hash_first(&it, hashes[idx]);
                while (hash_next(&it)) {
                    struct hash_item *item = hash_entry(hash_cur(&it), struct hash_item, elem);
                    printf("%d ", item->data);
                }
                printf("\n");
            }
            else if (idx < bm_count) {
                for (size_t i = 0; i < bitmap_size(bitmaps[idx]); i++) {
                    printf("%d", bitmap_test(bitmaps[idx], i) ? 1 : 0);
                }
                printf("\n");
            }
        }
            // Deletes a container and frees its memory
        else if (strcmp(command, "delete") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx == -1) continue;

            if (idx < list_count) {
                while (!list_empty(lists[idx])) {
                    struct list_elem *e = list_pop_front(lists[idx]);
                    free(list_entry(e, struct list_item, elem));
                }
                free(lists[idx]);
                for (int i = idx; i < list_count - 1; i++) {
                    lists[i] = lists[i + 1];
                    strcpy(names[i], names[i + 1]);
                }
                list_count--;
            }
            else if (idx < hash_count) {
                hash_destroy(hashes[idx], NULL);
                free(hashes[idx]);
                for (int i = idx; i < hash_count - 1; i++) {
                    hashes[i] = hashes[i + 1];
                    strcpy(names[i], names[i + 1]);
                }
                hash_count--;
            }
            else if (idx < bm_count) {
                bitmap_destroy(bitmaps[idx]);
                for (int i = idx; i < bm_count - 1; i++) {
                    bitmaps[i] = bitmaps[i + 1];
                    strcpy(names[i], names[i + 1]);
                }
                bm_count--;
            }
        }
            // Adds an element to the end of a list
        else if (strcmp(command, "list_push_back") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                struct list_item *item = malloc(sizeof(struct list_item));
                ASSERT(item != NULL);
                item->data = value;
                list_push_back(lists[idx], &item->elem);
            }
        }
            // Adds an element to the front of a list
        else if (strcmp(command, "list_push_front") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                struct list_item *item = malloc(sizeof(struct list_item));
                ASSERT(item != NULL);
                item->data = value;
                list_push_front(lists[idx], &item->elem);
            }
        }
            // Prints the first element of a list
        else if (strcmp(command, "list_front") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_item *item = list_entry(list_front(lists[idx]), struct list_item, elem);
                printf("%d\n", item->data);
            }
        }
            // Prints the last element of a list
        else if (strcmp(command, "list_back") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_item *item = list_entry(list_back(lists[idx]), struct list_item, elem);
                printf("%d\n", item->data);
            }
        }
            // Removes and frees the first element of a list
        else if (strcmp(command, "list_pop_front") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_elem *e = list_pop_front(lists[idx]);
                free(list_entry(e, struct list_item, elem));
            }
        }
            // Removes and frees the last element of a list
        else if (strcmp(command, "list_pop_back") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_elem *e = list_pop_back(lists[idx]);
                free(list_entry(e, struct list_item, elem));
            }
        }
            // Removes an element at a specific position in a list
        else if (strcmp(command, "list_remove") == 0) {
            scanf("%s %d", name, &pos1);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_elem *e = get_elem_at(lists[idx], pos1);
                if (e != list_end(lists[idx])) {
                    list_remove(e);
                    free(list_entry(e, struct list_item, elem));
                }
            }
        }
            // Reverses the order of a list
        else if (strcmp(command, "list_reverse") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                list_reverse(lists[idx]);
            }
        }
            // Swaps two elements in a list
        else if (strcmp(command, "list_swap") == 0) {
            scanf("%s %d %d", name, &pos1, &pos2);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                struct list_elem *e1 = get_elem_at(lists[idx], pos1);
                struct list_elem *e2 = get_elem_at(lists[idx], pos2);
                if (e1 != list_end(lists[idx]) && e2 != list_end(lists[idx])) {
                    list_swap(e1, e2);
                }
            }
        }
            // Randomly shuffles a list
        else if (strcmp(command, "list_shuffle") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                list_shuffle(lists[idx]);
            }
        }
            // Sorts a list in ascending order
        else if (strcmp(command, "list_sort") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                list_sort(lists[idx], list_less, NULL);
            }
        }
            // Removes duplicates from a list, optionally comparing with another list
        else if (strcmp(command, "list_unique") == 0) {
            scanf("%s", name);
            int idx1 = find_container(name);
            if (idx1 == -1 || idx1 >= list_count) continue;

            struct list *other = NULL;
            char c = getchar();
            if (c == ' ' && scanf("%s", name2) == 1) {
                int idx2 = find_container(name2);
                if (idx2 != -1 && idx2 < list_count) {
                    other = lists[idx2];
                }
            }
            list_unique(lists[idx1], other, list_less, NULL);
        }
            // Moves a range of elements from one list to another
        else if (strcmp(command, "list_splice") == 0) {
            char list1_name[20], list2_name[20];
            int pos1, start, cnt;
            scanf("%s %d %s %d %d", list1_name, &pos1, list2_name, &start, &cnt);

            int idx1 = find_container(list1_name);
            int idx2 = find_container(list2_name);

            if (idx1 != -1 && idx1 < list_count && idx2 != -1 && idx2 < list_count) {
                struct list_elem *before = get_elem_at(lists[idx1], pos1);
                struct list_elem *first = get_elem_at(lists[idx2], start);
                if (before != list_end(lists[idx1]) && first != list_end(lists[idx2])) {
                    struct list_elem *last = first;
                    for (int i = 0; i < cnt - 1 && last != list_end(lists[idx2]); i++) {
                        last = list_next(last);
                    }
                    if (last != list_end(lists[idx2])) {
                        list_splice(before, first, last);
                    }
                }
            }
        }
            // Checks if a list is empty
        else if (strcmp(command, "list_empty") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                printf("%s\n", list_empty(lists[idx]) ? "true" : "false");
            }
        }
            // Prints the size of a list
        else if (strcmp(command, "list_size") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                printf("%zu\n", list_size(lists[idx]));
            }
        }
            // Prints the maximum value in a list
        else if (strcmp(command, "list_max") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_elem *max = list_max(lists[idx], list_less, NULL);
                struct list_item *item = list_entry(max, struct list_item, elem);
                printf("%d\n", item->data);
            }
        }
            // Prints the minimum value in a list
        else if (strcmp(command, "list_min") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count && !list_empty(lists[idx])) {
                struct list_elem *min = list_min(lists[idx], list_less, NULL);
                struct list_item *item = list_entry(min, struct list_item, elem);
                printf("%d\n", item->data);
            }
        }
            // Inserts an element at a specific position in a list
        else if (strcmp(command, "list_insert") == 0) {
            scanf("%s %d %d", name, &pos1, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                struct list_item *item = malloc(sizeof(struct list_item));
                ASSERT(item != NULL);
                item->data = value;
                struct list_elem *e = get_elem_at(lists[idx], pos1);
                if (e != list_end(lists[idx])) {
                    list_insert(e, &item->elem);
                } else {
                    list_push_back(lists[idx], &item->elem);
                }
            }
        }
            // Inserts an element in sorted order into a list
        else if (strcmp(command, "list_insert_ordered") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < list_count) {
                struct list_item *item = malloc(sizeof(struct list_item));
                ASSERT(item != NULL);
                item->data = value;
                struct list_elem *e = list_begin(lists[idx]);
                while (e != list_end(lists[idx]) && list_entry(e, struct list_item, elem)->data < value) {
                    e = list_next(e);
                }
                if (e == list_end(lists[idx])) {
                    list_push_back(lists[idx], &item->elem);
                } else {
                    list_insert(e, &item->elem);
                }
            }
        }
            // Inserts an element into a hash table
        else if (strcmp(command, "hash_insert") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                struct hash_item *item = malloc(sizeof(struct hash_item));
                ASSERT(item != NULL);
                item->data = value;
                hash_insert(hashes[idx], &item->elem);
            }
        }
            // Checks if a hash table is empty
        else if (strcmp(command, "hash_empty") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                printf("%s\n", hash_empty(hashes[idx]) ? "true" : "false");
            }
        }
            // Prints the size of a hash table
        else if (strcmp(command, "hash_size") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                printf("%zu\n", hash_size(hashes[idx]));
            }
        }
            // Clears all elements from a hash table
        else if (strcmp(command, "hash_clear") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                hash_clear(hashes[idx], NULL);
            }
        }
            // Finds and prints an element in a hash table
        else if (strcmp(command, "hash_find") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                struct hash_item tmp;
                tmp.data = value;
                struct hash_elem *e = hash_find(hashes[idx], &tmp.elem);
                if (e != NULL) {
                    struct hash_item *item = hash_entry(e, struct hash_item, elem);
                    printf("%d\n", item->data);
                }
            }
        }
            // Deletes an element from a hash table
        else if (strcmp(command, "hash_delete") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                struct hash_item tmp;
                tmp.data = value;
                struct hash_elem *e = hash_delete(hashes[idx], &tmp.elem);
                if (e != NULL) {
                    free(hash_entry(e, struct hash_item, elem));
                }
            }
        }
            // Applies a function (square or triple) to all hash elements
        else if (strcmp(command, "hash_apply") == 0) {
            scanf("%s %s", name, func);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                if (strcmp(func, "square") == 0) {
                    hash_apply(hashes[idx], hash_square);
                } else if (strcmp(func, "triple") == 0) {
                    hash_apply(hashes[idx], hash_triple);
                }
            }
        }
            // Sets a bit to 1 in a bitmap
        else if (strcmp(command, "bitmap_mark") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bitmap_mark(bitmaps[idx], value);
            }
        }
            // Sets a bit to 0 in a bitmap
        else if (strcmp(command, "bitmap_reset") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bitmap_reset(bitmaps[idx], value);
            }
        }
            // Toggles a bit in a bitmap
        else if (strcmp(command, "bitmap_flip") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bitmap_flip(bitmaps[idx], value);
            }
        }
            // Sets a bit to a specific value in a bitmap
        else if (strcmp(command, "bitmap_set") == 0) {
            scanf("%s %d %s", name, &value, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                bitmap_set(bitmaps[idx], value, bool_val);
            }
        }
            // Sets multiple bits to a specific value in a bitmap
        else if (strcmp(command, "bitmap_set_multiple") == 0) {
            scanf("%s %d %d %s", name, &start, &cnt, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                bitmap_set_multiple(bitmaps[idx], start, cnt, bool_val);
            }
        }
            // Checks if all bits in a range are 1
        else if (strcmp(command, "bitmap_all") == 0) {
            scanf("%s %d %d", name, &start, &cnt);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool result = bitmap_all(bitmaps[idx], start, cnt);
                printf("%s\n", result ? "true" : "false");
            }
        }
            // Checks the value of a specific bit
        else if (strcmp(command, "bitmap_test") == 0) {
            scanf("%s %d", name, &pos1);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool result = bitmap_test(bitmaps[idx], pos1);
                printf("%s\n", result ? "true" : "false");
            }
        }
            // Checks if all bits in a range are 0
        else if (strcmp(command, "bitmap_none") == 0) {
            scanf("%s %d %d", name, &start, &cnt);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool result = bitmap_none(bitmaps[idx], start, cnt);
                printf("%s\n", result ? "true" : "false");
            }
        }
            // Dumps the bitmap contents in a formatted way
        else if (strcmp(command, "bitmap_dump") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bitmap_dump(bitmaps[idx]);
            }
        }
            // Prints the size of a bitmap
        else if (strcmp(command, "bitmap_size") == 0) {
            scanf("%s", name);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                printf("%zu\n", bitmap_size(bitmaps[idx]));
            }
        }
            // Counts bits with a specific value in a range
        else if (strcmp(command, "bitmap_count") == 0) {
            scanf("%s %d %d %s", name, &start, &cnt, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                size_t result = bitmap_count(bitmaps[idx], start, cnt, bool_val);
                printf("%zu\n", result);
            }
        }
            // Checks if any bit in a range is 1
        else if (strcmp(command, "bitmap_any") == 0) {
            scanf("%s %d %d", name, &start, &cnt);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool result = bitmap_any(bitmaps[idx], start, cnt);
                printf("%s\n", result ? "true" : "false");
            }
        }
            // Checks if a range contains only a specific value
        else if (strcmp(command, "bitmap_contains") == 0) {
            scanf("%s %d %d %s", name, &start, &cnt, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                bool result = bitmap_contains(bitmaps[idx], start, cnt, bool_val);
                printf("%s\n", result ? "true" : "false");
            }
        }
            // Expands the size of a bitmap
        else if (strcmp(command, "bitmap_expand") == 0) {
            scanf("%s %d", name, &size);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bitmaps[idx] = bitmap_expand(bitmaps[idx], size);
            }
        }
            // Sets all bits in a bitmap to a specific value
        else if (strcmp(command, "bitmap_set_all") == 0) {
            scanf("%s %s", name, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                bitmap_set_all(bitmaps[idx], bool_val);
            }
        }
            // Scans for a range of bits with a specific value
        else if (strcmp(command, "bitmap_scan") == 0) {
            scanf("%s %d %d %s", name, &start, &cnt, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                size_t result = bitmap_scan(bitmaps[idx], start, cnt, bool_val);
                printf("%zu\n", result);
            }
        }
            // Scans and flips a range of bits
        else if (strcmp(command, "bitmap_scan_and_flip") == 0) {
            scanf("%s %d %d %s", name, &start, &cnt, bool_str);
            int idx = find_container(name);
            if (idx != -1 && idx < bm_count) {
                bool_val = (strcmp(bool_str, "true") == 0);
                size_t result = bitmap_scan_and_flip(bitmaps[idx], start, cnt, bool_val);
                printf("%zu\n", result);
            }
        }
            // Replaces an element in a hash table
        else if (strcmp(command, "hash_replace") == 0) {
            scanf("%s %d", name, &value);
            int idx = find_container(name);
            if (idx != -1 && idx < hash_count) {
                struct hash_item *item = malloc(sizeof(struct hash_item));
                item->data = value;
                struct hash_elem *old = hash_replace(hashes[idx], &item->elem);
                if (old) free(hash_entry(old, struct hash_item, elem));
            }
        }
    }

    // Frees all remaining lists
    for (int i = 0; i < list_count; i++) {
        while (!list_empty(lists[i])) {
            struct list_elem *e = list_pop_front(lists[i]);
            free(list_entry(e, struct list_item, elem));
        }
        free(lists[i]);
    }
    // Frees all remaining hash tables
    for (int i = 0; i < hash_count; i++) {
        hash_destroy(hashes[i], NULL);
        free(hashes[i]);
    }
    // Frees all remaining bitmaps
    for (int i = 0; i < bm_count; i++) {
        bitmap_destroy(bitmaps[i]);
    }
    return 0;
}