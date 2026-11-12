# TP intégrateur — Mini-analyseur d'expressions en C

## Utilisation
```
make          # compile tout (tests avec -std=c11 -Wall -Wextra -Wpedantic -Werror
              #   -fsanitize=address,undefined ; bench en -O2 sans sanitizers)
make test     # lance tous les tests, pour les DEUX implémentations
make bench    # étape 7 : compare tableau et liste chaînée
./calc "max(1+2, 4*5) - -3^2"     # analyseur interactif (ou ./calc sans argument)
```
Aucune fuite mémoire (LeakSanitizer) ni comportement indéfini détecté.

## Organisation
| Fichier | Rôle |
|---|---|
| `token.h` | type de jeton commun aux piles et files |
| `stack.h`, `queue.h` | interfaces TAD (fournies) |
| `stack_array.c`, `stack_list.c` | étape 1 : pile tableau dynamique / liste chaînée |
| `queue_array.c`, `queue_list.c` | étape 2 : file tableau circulaire / liste chaînée |
| `expr.c/.h` | étapes 3 à 6 : parenthésage, lexer, infixe→postfixe, évaluation |
| `test_stack.c`, `test_queue.c`, `test_expr.c` | tests (vide, pop à vide, croissance > 100, destruction, erreurs) |
| `bench.c` | étape 7 |
| `calc.c` | petite interface en ligne de commande |
| `contacts.c/.h`, `test_contacts.c` | mini-projet : annuaire (27 listes doublement chaînées) |

## Choix de conception
- **Parenthésage** : pile de jetons contenant la position de chaque ouvrante. Gère `() [] {}`.
  Position renvoyée : fermante fautive, ou ouvrante non fermée la plus à gauche.
- **Priorités** : `+ -` (1) < `* /` (2) < moins unaire (3) < `^` (4). `^` et le moins unaire sont associatifs à droite, donc `2^3^2 = 512` et `-2^2 = -4`.
- **Moins unaire** : le lexer le détecte (jeton précédent absent, opérateur, `(` ou `,`) et émet l'opérateur `neg`. Un opérateur préfixe ne dépile jamais rien, ce qui rend `2^-1` correct.
- **Erreurs** : chaque fonction renvoie un statut et la position de la première erreur (`expr_strerror` donne le message). Cas couverts : parenthèse, caractère invalide, opérande manquante, opérandes en trop, division par zéro, domaine (`sqrt(-1)`, dépassement), variable sans valeur.
- **Extensions faites** : moins unaire, variables `a..z` (tableau de 26 valeurs), fonctions `sin cos sqrt abs max min` (avec virgule), `^` associatif à droite.

## Étape 7 — Mesures (5 000 000 éléments, -O2, une exécution)
| Opération | Tableau | Liste chaînée |
|---|---|---|
| Pile : push | 0,067 s | 0,127 s |
| Pile : pop | 0,013 s | 0,031 s |
| Pile : push/pop alternés (taille stable) | 0,067 s | 0,270 s |
| File : enqueue | 0,189 s | 0,045 s |
| File : dequeue | 0,023 s | 0,047 s |
| File : enqueue/dequeue alternés | 0,279 s | 0,277 s |

(Vos temps varieront selon la machine ; relancez `make bench`.)

**Commentaire**
- Pile : le tableau gagne nettement. Les éléments sont contigus (bon usage du cache) et il n'y a pas de `malloc`/`free` par élément. La liste paie un appel à l'allocateur à chaque opération.
- File : à l'enfilage massif, le tableau circulaire est plus lent ici car il doit se réagrandir (copie des éléments à chaque doublement). La liste a un coût constant par élément mais fragmente la mémoire. En régime stable (taille petite), les deux se valent : le tableau circulaire n'alloue plus rien, la liste réutilise l'allocateur.
- Complexité : push/pop/enqueue/dequeue sont en O(1) pour les deux. Le tableau est en O(1) *amorti* (les agrandissements sont rares mais coûteux), la liste en O(1) strict.
- Pour l'analyseur (petites expressions), la différence est négligeable. Le tableau est préférable en général, la liste intéressante si la latence par opération doit être régulière.

## Mini-projet : annuaire
- Tableau de 27 entrées (A–Z, `#` pour tout le reste), chacune pointant vers une liste doublement chaînée triée sans tenir compte de la casse.
- Opérations : `dir_add`, `dir_delete`, `dir_search`, `dir_update`, `dir_print`. Constructeurs/destructeurs/affichage pour `contact`, `node`, `list`, `directory`.
- Lien avec la deque chaînée : la liste a `head`/`tail`/`prev`/`next` et fournit `list_push_front/back` et `list_pop_front/back` en O(1). C'est exactement une deque chaînée ; l'insertion triée et la suppression d'un nœud quelconque s'appuient sur le même mécanisme de liens.

## Non fait
- Extension « remplacer les structures par une deque / fenêtre glissante » (marquée « lorsque pertinent » dans l'énoncé).
- Dépôt Git : à initialiser vous-même (`git init`, puis commit par étape).
