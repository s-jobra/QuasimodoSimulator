#include "htab.h"

#define AVG_LEN_MAX 2 // Max allowed average list length
#define RESIZE_COEF 2

/**
 * Obtains a properly casted key from the measure item ptr
 */
#define htab_m_get_key(item_p) ((htab_m_key_t)(item_p->data.key))

static uint64_t my_str_hash(const char *str) {
    uint64_t hash = 0;
    int c;
    while (c = *str++) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

htab_t* htab_init(size_t n)
{
    htab_t *t = (htab_t*) my_malloc(sizeof(htab_t));

    t->arr_ptr = (htab_item_t**)my_malloc(sizeof(htab_item_t*) * n);
    memset(t->arr_ptr, 0, sizeof(htab_item_t*) * n);

    t->size = 0;
    t->arr_size = n;

    return t;
}

/**
 * Measure table item dealloc
 */
static void htab_m_del_item(htab_item_t *i)
{
    free(i->data.key);
    free(i);
}

/**
 * Deletes all table items
 */
static void htab_clear(htab_t *t, void (*del_item_func)(htab_item_t*))
{
    htab_item_t *curr;

    for (size_t i = 0; i < t->arr_size; i++) {
        while (t->arr_ptr[i] != NULL) {
            curr = t->arr_ptr[i];
            t->arr_ptr[i] = t->arr_ptr[i]->next;
            del_item_func(curr);
        }
    }
    t->size = 0;
}

/**
 * Deletes the whole table
 */
static void htab_free(htab_t *t, void (*del_item_func)(htab_item_t*))
{
    if (t->arr_ptr != NULL) {
        htab_clear(t, del_item_func);
        free(t->arr_ptr);
    }
    free(t);
}

void htab_m_clear(htab_t *t)
{
    htab_clear(t, htab_m_del_item);
}

void htab_m_free(htab_t *t)
{
    htab_clear(t, htab_m_del_item);
}

/**
 * The hash function used when operating with measure hash table items
 */
static size_t htab_m_hash_func(htab_key_t key_raw) {
    return my_str_hash((htab_m_key_t) key_raw);
}

/**
 * Changes the size of the table's array and moves items from the original lists (size must be >0)
 */
static void htab_resize(htab_t *t, size_t newn, size_t (*hash_func)(htab_key_t))
{
    assert(newn != 0);

    // alloc and init new array
    htab_item_t **new_arr_ptr;
    new_arr_ptr = (htab_item_t**)my_malloc(newn * sizeof(htab_item_t*));
    memset(new_arr_ptr, 0, newn * sizeof(htab_item_t*));

    // index change
    htab_item_t *curr;
    htab_item_t *temp; // aux ptr to the elem in the new arr
    size_t new_index; // aux var for the new index of the current elem
    
    for (size_t i = 0; i < t->arr_size; i++) {
        curr = t->arr_ptr[i];

        while (curr != NULL) {
            new_index = hash_func(curr->data.key) % newn;

            // insert into new array
            if (new_arr_ptr[new_index] == NULL) {
                new_arr_ptr[new_index] = curr;
                curr = curr->next; //preserve the original list
                new_arr_ptr[new_index]->next = NULL;
            }
            else {
                temp = new_arr_ptr[new_index];
                while (temp->next != NULL) {
                    temp = temp->next;
                }
                temp->next = curr;
                curr = curr->next; // preserve the original list
                temp->next->next = NULL;
            }
        }
    }

    free(t->arr_ptr);
    t->arr_ptr = new_arr_ptr;
    t->arr_size = newn;
    return;
}

void htab_m_lookup_add(htab_t *t, htab_m_key_t key)
{
    htab_item_t *item = t->arr_ptr[htab_m_hash_func(key) % t->arr_size];

    // find the item
    while (item != NULL) {
        if (strcmp((const char*)(item->data.key),key) == 0) {
            item->data.value++;
            return;
        }
        item = item->next;
    }

    item = (htab_item_t*)my_malloc(sizeof(htab_item_t));
    // item init
    htab_m_key_t key_temp = (htab_m_key_t)my_malloc(sizeof(char) * (strlen(key) + 1));
    item->data.key = (htab_key_t)(strcpy(key_temp, key));
    item->data.value = 1;
    item->next = NULL;

    // insert new item into the table
    size_t new_index = htab_m_hash_func(key) % t->arr_size;
    if (t->arr_ptr[new_index] == NULL) {
        t->arr_ptr[new_index] = item;
    }
    else {
        htab_item_t *temp = t->arr_ptr[new_index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = item;
    }

    t->size++;
    if (((t->size + 0.0) / t->arr_size) > AVG_LEN_MAX) { // +0.0 because of non-integer division
        htab_resize(t, t->size * RESIZE_COEF, htab_m_hash_func);
    }
}

void htab_m_print_all(htab_t *t, FILE *output)
{
    fprintf(output, "Sampled results:\n");

    htab_item_t *curr;
    for (size_t i = 0; i < t->arr_size; i++) {
        curr = t->arr_ptr[i];

        while (curr != NULL) {
            fprintf(output,"    \'");
            // key stored as LSBF
            for(int i=strlen((const char*)(curr->data.key)); i >= 0 ;i--) {
                putc(htab_m_get_key(curr)[i], output);
            }
            fprintf(output,"\'    %d\n", curr->data.value);
            curr = curr->next;
        }
    }
    return;
}

/* end of "htab.c" */