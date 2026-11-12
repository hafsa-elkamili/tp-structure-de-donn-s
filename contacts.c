#include <ctype.h>
#include <stdlib.h>
#include "contacts.h"

/* ---------- utilitaires ---------- */

static int ci_cmp(const char *a, const char *b)       /* comparaison sans casse */
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static void copy_str(char *dst, size_t n, const char *src)
{
    size_t i = 0;
    for (; i + 1 < n && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

/* ---------- contact ---------- */

contact *contact_new(const char *name, const char *phone)
{
    contact *c = malloc(sizeof *c);
    if (!c) return NULL;
    copy_str(c->name, sizeof c->name, name);
    copy_str(c->phone, sizeof c->phone, phone);
    return c;
}

void contact_free(contact *c) { free(c); }

void contact_print(const contact *c, FILE *f)
{
    fprintf(f, "%s : %s", c->name, c->phone);
}

/* ---------- noeud ---------- */

node *node_new(contact *c)
{
    node *n = malloc(sizeof *n);
    if (!n) return NULL;
    n->c = c;
    n->prev = n->next = NULL;
    return n;
}

void node_free(node *n)
{
    if (!n) return;
    contact_free(n->c);
    free(n);
}

void node_print(const node *n, FILE *f) { contact_print(n->c, f); }

/* ---------- liste doublement chainee ---------- */

list *list_new(void)
{
    list *l = malloc(sizeof *l);
    if (!l) return NULL;
    l->head = l->tail = NULL;
    l->size = 0;
    return l;
}

void list_free(list *l)
{
    if (!l) return;
    node *n = l->head;
    while (n) {
        node *next = n->next;
        node_free(n);
        n = next;
    }
    free(l);
}

void list_print(const list *l, FILE *f)
{
    for (const node *n = l->head; n; n = n->next) {
        fputs("  - ", f);
        node_print(n, f);
        fputc('\n', f);
    }
}

int list_push_front(list *l, contact *c)
{
    node *n = node_new(c);
    if (!n) return -1;
    n->next = l->head;
    if (l->head) l->head->prev = n; else l->tail = n;
    l->head = n;
    l->size++;
    return 0;
}

int list_push_back(list *l, contact *c)
{
    node *n = node_new(c);
    if (!n) return -1;
    n->prev = l->tail;
    if (l->tail) l->tail->next = n; else l->head = n;
    l->tail = n;
    l->size++;
    return 0;
}

static contact *detach(list *l, node *n)      /* retire n, rend son contact */
{
    contact *c = n->c;
    if (n->prev) n->prev->next = n->next; else l->head = n->next;
    if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
    free(n);
    l->size--;
    return c;
}

contact *list_pop_front(list *l) { return l->head ? detach(l, l->head) : NULL; }
contact *list_pop_back(list *l)  { return l->tail ? detach(l, l->tail) : NULL; }

int list_insert_sorted(list *l, contact *c)
{
    node *cur = l->head;
    while (cur && ci_cmp(cur->c->name, c->name) <= 0) cur = cur->next;
    if (!cur) return list_push_back(l, c);          /* fin de liste (ou liste vide) */
    if (!cur->prev) return list_push_front(l, c);   /* avant le premier */

    node *n = node_new(c);
    if (!n) return -1;
    n->prev = cur->prev;
    n->next = cur;
    cur->prev->next = n;
    cur->prev = n;
    l->size++;
    return 0;
}

node *list_find(const list *l, const char *name)
{
    for (node *n = l->head; n; n = n->next) {
        int d = ci_cmp(n->c->name, name);
        if (d == 0) return n;
        if (d > 0) break;                           /* liste triee : inutile d'aller plus loin */
    }
    return NULL;
}

void list_remove(list *l, node *n) { contact_free(detach(l, n)); }

/* ---------- annuaire ---------- */

static size_t dir_index(const char *name)
{
    unsigned char c = (unsigned char)name[0];
    return isalpha(c) ? (size_t)(toupper(c) - 'A') : DIR_SIZE - 1;
}

directory *dir_new(void)
{
    directory *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    for (size_t i = 0; i < DIR_SIZE; i++) {
        d->slots[i] = list_new();
        if (!d->slots[i]) { dir_free(d); return NULL; }
    }
    return d;
}

void dir_free(directory *d)
{
    if (!d) return;
    for (size_t i = 0; i < DIR_SIZE; i++) list_free(d->slots[i]);   /* list_free(NULL) ok */
    free(d);
}

void dir_print(const directory *d, FILE *f)
{
    fprintf(f, "Annuaire (%zu contact%s)\n", d->count, d->count > 1 ? "s" : "");
    for (size_t i = 0; i < DIR_SIZE; i++) {
        if (d->slots[i]->size == 0) continue;
        fprintf(f, "[%c]\n", i == DIR_SIZE - 1 ? '#' : (char)('A' + i));
        list_print(d->slots[i], f);
    }
}

int dir_add(directory *d, const char *name, const char *phone)
{
    if (!name || !*name || !phone) return DIR_ERR_ARG;
    list *l = d->slots[dir_index(name)];
    if (list_find(l, name)) return DIR_DUPLICATE;
    contact *c = contact_new(name, phone);
    if (!c) return DIR_ERR_MEM;
    if (list_insert_sorted(l, c) != 0) { contact_free(c); return DIR_ERR_MEM; }
    d->count++;
    return DIR_OK;
}

int dir_delete(directory *d, const char *name)
{
    if (!name || !*name) return DIR_NOT_FOUND;
    list *l = d->slots[dir_index(name)];
    node *n = list_find(l, name);
    if (!n) return DIR_NOT_FOUND;
    list_remove(l, n);
    d->count--;
    return DIR_OK;
}

const contact *dir_search(const directory *d, const char *name)
{
    if (!name || !*name) return NULL;
    node *n = list_find(d->slots[dir_index(name)], name);
    return n ? n->c : NULL;
}

int dir_update(directory *d, const char *name, const char *new_phone)
{
    if (!new_phone) return DIR_ERR_ARG;
    if (!name || !*name) return DIR_NOT_FOUND;
    node *n = list_find(d->slots[dir_index(name)], name);
    if (!n) return DIR_NOT_FOUND;
    copy_str(n->c->phone, sizeof n->c->phone, new_phone);
    return DIR_OK;
}
