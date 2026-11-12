#ifndef CONTACTS_H
#define CONTACTS_H
#include <stddef.h>
#include <stdio.h>

/* Mini-projet : annuaire = tableau de 27 entrees (A-Z + '#' pour les autres),
   chaque entree pointe vers une liste DOUBLEMENT chainee triee par nom.

   La liste est aussi une deque chainee : push/pop aux deux extremites en O(1)
   grace aux pointeurs head, tail, prev et next. */

#define DIR_SIZE 27

typedef struct contact {
    char name[48];
    char phone[24];
} contact;

typedef struct node {
    contact *c;                 /* le noeud possede le contact */
    struct node *prev, *next;
} node;

typedef struct list {
    node *head, *tail;
    size_t size;
} list;

typedef struct directory {
    list *slots[DIR_SIZE];
    size_t count;
} directory;

/* contact */
contact *contact_new(const char *name, const char *phone);
void     contact_free(contact *c);
void     contact_print(const contact *c, FILE *f);

/* noeud */
node    *node_new(contact *c);              /* prend possession de c */
void     node_free(node *n);                /* libere aussi le contact */
void     node_print(const node *n, FILE *f);

/* liste doublement chainee / deque */
list    *list_new(void);
void     list_free(list *l);
void     list_print(const list *l, FILE *f);
int      list_push_front(list *l, contact *c);   /* 0 ou -1 */
int      list_push_back(list *l, contact *c);
contact *list_pop_front(list *l);                /* NULL si vide ; l'appelant libere */
contact *list_pop_back(list *l);
int      list_insert_sorted(list *l, contact *c);
node    *list_find(const list *l, const char *name);
void     list_remove(list *l, node *n);          /* detache et libere n */

/* annuaire */
#define DIR_OK         0
#define DIR_NOT_FOUND  1
#define DIR_DUPLICATE  1
#define DIR_ERR_MEM   (-1)
#define DIR_ERR_ARG   (-2)

directory      *dir_new(void);
void            dir_free(directory *d);
void            dir_print(const directory *d, FILE *f);
int             dir_add(directory *d, const char *name, const char *phone);
int             dir_delete(directory *d, const char *name);
const contact  *dir_search(const directory *d, const char *name);
int             dir_update(directory *d, const char *name, const char *new_phone);

#endif
