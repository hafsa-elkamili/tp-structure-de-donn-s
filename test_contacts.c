#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "contacts.h"

int main(void)
{
    /* ---- liste / deque ---- */
    list *l = list_new();
    assert(l && list_pop_front(l) == NULL && list_pop_back(l) == NULL);
    list_push_back(l, contact_new("b", "2"));
    list_push_front(l, contact_new("a", "1"));
    list_push_back(l, contact_new("c", "3"));
    assert(l->size == 3 && strcmp(l->head->c->name, "a") == 0 && strcmp(l->tail->c->name, "c") == 0);
    contact *c = list_pop_back(l);  assert(strcmp(c->name, "c") == 0); contact_free(c);
    c = list_pop_front(l);          assert(strcmp(c->name, "a") == 0); contact_free(c);
    c = list_pop_front(l);          assert(strcmp(c->name, "b") == 0); contact_free(c);
    assert(l->size == 0 && l->head == NULL && l->tail == NULL);
    list_free(l);

    /* ---- annuaire ---- */
    directory *d = dir_new();
    assert(d && d->count == 0);
    assert(dir_add(d, "Alice", "0600000001") == DIR_OK);
    assert(dir_add(d, "alain", "0600000002") == DIR_OK);        /* meme case A, tri insensible a la casse */
    assert(dir_add(d, "Albert", "0600000003") == DIR_OK);
    assert(dir_add(d, "Zoe", "0600000004") == DIR_OK);
    assert(dir_add(d, "3615 Pizza", "0600000005") == DIR_OK);   /* case '#' */
    assert(dir_add(d, "Émile", "0600000006") == DIR_OK);        /* octet non ASCII -> '#' */
    assert(dir_add(d, "ALICE", "x") == DIR_DUPLICATE);
    assert(dir_add(d, "", "x") == DIR_ERR_ARG);
    assert(d->count == 6);

    /* ordre (insensible a la casse) dans la liste A : alain, Albert, Alice */
    list *a = d->slots[0];
    assert(a->size == 3);
    assert(strcmp(a->head->c->name, "alain") == 0);
    assert(strcmp(a->head->next->c->name, "Albert") == 0);
    assert(strcmp(a->tail->c->name, "Alice") == 0);
    assert(a->tail->prev == a->head->next && a->head->next->prev == a->head);   /* liens prev coherents */
    assert(d->slots[25]->size == 1 && d->slots[DIR_SIZE - 1]->size == 2);

    /* recherche */
    const contact *f = dir_search(d, "alice");
    assert(f && strcmp(f->phone, "0600000001") == 0);
    assert(dir_search(d, "Bob") == NULL);
    assert(dir_search(d, "") == NULL);

    /* mise a jour */
    assert(dir_update(d, "ALAIN", "0700000000") == DIR_OK);
    assert(strcmp(dir_search(d, "alain")->phone, "0700000000") == 0);
    assert(dir_update(d, "Bob", "1") == DIR_NOT_FOUND);

    /* suppression : milieu, tete, queue, inexistant */
    assert(dir_delete(d, "ALBERT") == DIR_OK);                    /* milieu */
    assert(a->size == 2 && a->head->next == a->tail && a->tail->prev == a->head);
    assert(dir_delete(d, "alain") == DIR_OK);                     /* tete */
    assert(dir_delete(d, "Alice") == DIR_OK);                     /* dernier */
    assert(a->size == 0 && a->head == NULL && a->tail == NULL);
    assert(dir_delete(d, "Alice") == DIR_NOT_FOUND);
    assert(d->count == 3);

    dir_print(d, stdout);
    dir_free(d);          /* fuites verifiees par LeakSanitizer */
    dir_free(NULL);
    printf("test_contacts : OK\n");
    return 0;
}
