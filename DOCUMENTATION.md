# Documentation technique — Mini-analyseur d'expressions en C

TP intégrateur « Structures de données — Piles et files ».
Ce document décrit l'architecture, les interfaces, les algorithmes, les tests et les mesures.

---

## 1. Vue d'ensemble

Le programme analyse une expression arithmétique écrite en notation infixe, la transforme en notation postfixe, puis l'évalue :

```
chaîne  -->  parenthésage  -->  lexer  -->  infixe→postfixe  -->  évaluation  -->  résultat
 "3+4*2"     (pile)         (file de     (pile d'opérateurs    (pile de
                              jetons)     + file de sortie)      valeurs)
```

Deux TAD sont utilisés partout : la **pile** et la **file**. Chacun existe en deux implémentations interchangeables (tableau et liste chaînée) derrière la même interface. L'analyseur ne dépend que des interfaces `stack.h` et `queue.h`.

## 2. Compilation et utilisation

| Commande | Effet |
|---|---|
| `make` | compile les tests (2 implémentations), les benchs, `test_contacts` et `calc` |
| `make test` | exécute tous les tests |
| `make bench` | compare tableau et liste chaînée (étape 7) |
| `./calc "expr"` | évalue une expression ; sans argument, mode interactif |
| `make clean` | supprime les exécutables |

Options de compilation des tests : `-std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. Le bench utilise `-O2` sans sanitizers pour des temps représentatifs. Prérequis : `gcc` et `make` (la bibliothèque `-lm` est liée pour `pow`, `sqrt`, etc.).

Mode interactif de `calc` :
```
> let x 3
> x^2 + 1
postfixe : x 2 ^ 1 +
resultat : 10
> (1+2
erreur : parenthese non appariee
  (1+2
  ^
```

## 3. Arborescence

```
tp/
├── token.h                  type de jeton commun
├── stack.h  queue.h         interfaces des TAD
├── stack_array.c            pile : tableau dynamique
├── stack_list.c             pile : liste chaînée
├── queue_array.c            file : tableau circulaire
├── queue_list.c             file : liste chaînée
├── expr.h  expr.c           parenthésage, lexer, postfixe, évaluation
├── calc.c                   interface en ligne de commande
├── test_stack.c  test_queue.c  test_expr.c
├── bench.c                  mesures de performance
├── contacts.h  contacts.c  test_contacts.c      mini-projet annuaire
├── Makefile
├── README.md                résumé
└── DOCUMENTATION.md         ce document
```

## 4. Le type jeton (`token.h`)

```c
typedef enum { T_NUM, T_VAR, T_OP, T_FUNC, T_LP, T_RP, T_COMMA } token_type;

typedef struct {
    token_type type;
    double val;      /* T_NUM : valeur                                  */
    char op;         /* T_OP : + - * / ^ ou 'u' (moins unaire)
                        T_VAR : lettre de la variable                    */
    char name[8];    /* T_FUNC : sin cos sqrt abs max min               */
    int pos;         /* indice dans la chaîne source (messages d'erreur) */
} token;
```

Le même type sert d'élément pour les piles et les files. Pour l'évaluation, la pile de valeurs contient des jetons `T_NUM`. Le champ `pos` permet de rapporter la position exacte d'une erreur, même après la conversion en postfixe.

## 5. TAD Pile (`stack.h`)

| Fonction | Rôle | Retour |
|---|---|---|
| `stack_new()` | crée une pile vide | pointeur, ou `NULL` si mémoire insuffisante |
| `stack_free(s)` | libère la pile et son contenu (accepte `NULL`) | — |
| `stack_push(s, t)` | empile | 0 ou -1 (mémoire) |
| `stack_pop(s, &out)` | dépile ; `out` peut être `NULL` | 0 ou -1 (pile vide) |
| `stack_top(s, &out)` | lit le sommet sans le retirer | 0 ou -1 (pile vide) |
| `stack_size(s)`, `stack_empty(s)` | taille, test de vacuité | — |
| `stack_impl()` | nom de l'implémentation (pour les tests/bench) | chaîne |

**`stack_array.c`** : tableau dynamique `data[0..size-1]`, capacité initiale 8, doublée par `realloc` quand il est plein. Si `realloc` échoue, l'ancien tableau reste valide et `push` renvoie -1. Coût : O(1) amorti.

**`stack_list.c`** : liste simplement chaînée, insertion et suppression en tête. Coût : O(1) strict, un `malloc`/`free` par élément.

## 6. TAD File (`queue.h`)

| Fonction | Rôle | Retour |
|---|---|---|
| `queue_new()` / `queue_free(q)` | création / destruction (accepte `NULL`) | — |
| `queue_enqueue(q, t)` | ajoute en fin de file | 0 ou -1 (mémoire) |
| `queue_dequeue(q, &out)` | retire le premier élément ; `out` peut être `NULL` | 0 ou -1 (file vide) |
| `queue_front(q, &out)` | lit le premier sans le retirer | 0 ou -1 |
| `queue_size(q)`, `queue_empty(q)`, `queue_impl()` | — | — |

**`queue_array.c`** : tableau circulaire. `head` est l'indice du premier élément, les éléments sont aux positions `(head + i) % cap`. L'enfilage écrit en `(head + size) % cap`. Quand la file est pleine, `grow` alloue un tableau deux fois plus grand et y recopie les éléments *dans l'ordre logique* (en repartant de `head = 0`). Ce « déroulement » est le point délicat : il fonctionne même quand `head != 0`, ce que `test_queue.c` vérifie.

**`queue_list.c`** : liste simplement chaînée avec deux pointeurs, `head` (sortie) et `tail` (entrée). Quand la file devient vide après un `dequeue`, `tail` est remis à `NULL`, sinon il pointerait vers un nœud libéré.

## 7. Analyseur (`expr.h` / `expr.c`)

### 7.1 Statuts d'erreur

Toutes les fonctions renvoient un `expr_status` et, via `int *errpos` (peut être `NULL`), l'indice de la première erreur dans la chaîne (ou -1).

| Statut | Signification | Exemple |
|---|---|---|
| `EXPR_OK` | succès | `3+4` |
| `EXPR_ERR_PAREN` | parenthèse non appariée ou de mauvais type | `(1+2`, `1+2)`, `(]` |
| `EXPR_ERR_TOKEN` | caractère, nombre ou identifiant invalide | `3 $ 4`, `1.2.3`, `foo(1)` |
| `EXPR_ERR_SYNTAX` | jeton mal placé (virgule hors fonction) | — |
| `EXPR_ERR_OPERAND` | opérandes insuffisantes pour un opérateur/fonction | `3 +`, `max(3)` |
| `EXPR_ERR_MALFORMED` | expression vide ou opérandes en trop | `""`, `3 4` |
| `EXPR_ERR_DIVZERO` | division par zéro | `1/0`, `1/(2-2)` |
| `EXPR_ERR_DOMAIN` | résultat hors domaine ou infini | `sqrt(-1)`, `10^1000` |
| `EXPR_ERR_VAR` | variable utilisée sans table de valeurs | `x+1` avec `vars == NULL` |
| `EXPR_ERR_MEM` | allocation échouée | — |

`expr_strerror(status)` renvoie un message lisible.

### 7.2 Étape 3 — `expr_check_brackets`

Algorithme classique : on parcourt la chaîne ; une ouvrante (`(`, `[`, `{`) est empilée avec sa position ; une fermante dépile et doit correspondre à l'ouvrante. Erreur si la pile est vide (fermante orpheline) ou si les types diffèrent. À la fin, s'il reste des ouvrantes, on signale la plus à gauche.

### 7.3 Étape 4 — `expr_tokenize`

Parcours gauche→droite, espaces ignorés :
- **Nombre** : suite de chiffres et de points (`12`, `3.5`, `.5`) convertie par `strtod`. Erreur si plusieurs points ou nombre trop long.
- **Identifiant** (lettres) : une seule lettre non suivie de `(` est une **variable** ; sinon il doit s'agir d'une **fonction** connue (`sin cos sqrt abs max min`), insensible à la casse.
- **Opérateurs** `+ - * / ^`, parenthèses et virgule.
- **Signe unaire** : le lexer garde un indicateur « le jeton précédent est un opérande (nombre, variable, `)`) ». Si ce n'est pas le cas, `-` devient l'opérateur unaire `u` et `+` est ignoré. Ainsi `-3+4`, `2*(-1)`, `2^-1` et `max(-1,2)` sont tous analysés correctement.

Le résultat est une file de jetons dans l'ordre de lecture.

### 7.4 Étape 5 — `expr_to_postfix` (algorithme de Dijkstra, « shunting-yard »)

Données : la file infixe en entrée, une **pile d'opérateurs**, une **file de sortie**.

| Jeton lu | Action |
|---|---|
| nombre, variable | enfiler en sortie |
| fonction | empiler |
| virgule | dépiler vers la sortie jusqu'à la `(` (non retirée) ; erreur si aucune |
| opérateur binaire *o1* | tant que le sommet est un opérateur *o2* avec `prio(o2) > prio(o1)`, ou `prio(o2) == prio(o1)` et *o1* associatif à gauche : dépiler *o2* vers la sortie ; puis empiler *o1* |
| opérateur unaire | empiler directement, sans rien dépiler (opérateur préfixe) |
| `(` | empiler |
| `)` | dépiler vers la sortie jusqu'à `(` ; erreur si absente ; retirer `(` ; si le sommet est une fonction, l'envoyer en sortie |
| fin | vider la pile vers la sortie ; erreur si une `(` reste |

**Priorités** : `+ -` = 1, `* /` = 2, moins unaire = 3, `^` = 4. Associatifs à droite : `^` et le moins unaire (`2^3^2 = 2^(3^2) = 512`). Comme le moins unaire est moins prioritaire que `^`, `-2^2 = -(2^2) = -4`.

**Exemple** — `3+4*2` :

| Lu | Pile d'opérateurs | Sortie |
|---|---|---|
| `3` | | `3` |
| `+` | `+` | `3` |
| `4` | `+` | `3 4` |
| `*` | `+ *` (prio 2 > 1, pas de dépilage) | `3 4` |
| `2` | `+ *` | `3 4 2` |
| fin | | `3 4 2 * +` |

### 7.5 Étape 6 — `expr_eval_postfix`

On lit la file postfixe avec une **pile de valeurs** :
- nombre → empiler ; variable → empiler `vars[lettre - 'a']` ;
- opérateur binaire → dépiler *b* puis *a*, empiler `a op b` ;
- moins unaire → dépiler, empiler l'opposé ;
- fonction → dépiler 1 argument (2 pour `max`/`min`), empiler le résultat.

Erreurs : pile insuffisante (`EXPR_ERR_OPERAND`), division par zéro, `sqrt` d'un négatif, résultat non fini (`EXPR_ERR_DOMAIN`). À la fin, la pile doit contenir **exactement une** valeur : sinon l'expression est mal formée (vide, ou plusieurs valeurs comme dans `3 4`).

Suite de l'exemple : `3 4 2 * +` → pile `3` ; `3 4` ; `3 4 2` ; `*` donne `3 8` ; `+` donne `11`.

### 7.6 Fonctions d'enchaînement

- `expr_eval(s, vars, &res, &pos)` : parenthésage, lexer, postfixe, évaluation, avec libération des files intermédiaires.
- `expr_to_postfix_string(s, buf, n, &pos)` : écrit la notation postfixe (`3 4 2 * +`, `neg` pour le moins unaire). Utile pour les tests et pour `calc`.

## 8. Tests

`make test` lance 7 exécutables (stack, queue et expr, chacun avec les deux implémentations, plus l'annuaire).

| Fichier | Ce qui est vérifié |
|---|---|
| `test_stack.c` | pile vide, `pop`/`top` à vide, ordre LIFO, croissance à 5000 éléments, `pop` avec `out == NULL`, destruction d'une pile non vide |
| `test_queue.c` | file vide, `dequeue` à vide, ordre FIFO, **enroulement** du tampon circulaire (5000 passages), croissance avec `head != 0`, réutilisation après vidage, destruction |
| `test_expr.c` | 7 cas de parenthésage, 9 conversions postfixes, 16 évaluations (priorités, associativité, unaire, variables, fonctions), 16 erreurs avec statut **et** position attendus |
| `test_contacts.c` | opérations de deque, ajout/doublon/tri insensible à la casse, cases A–Z et `#`, recherche, mise à jour, suppression en tête/milieu/queue, cohérence des liens `prev`/`next` |

AddressSanitizer / LeakSanitizer détectent fuites et accès invalides ; UBSan détecte les comportements indéfinis. Aucun n'a été relevé.

## 9. Étape 7 — Comparaison des implémentations

Mesures (5 000 000 éléments, `-O2`, une exécution sur la machine de développement) :

| Opération | Tableau | Liste chaînée |
|---|---|---|
| Pile : push | 0,067 s | 0,127 s |
| Pile : pop | 0,013 s | 0,031 s |
| Pile : push/pop alternés (taille stable) | 0,067 s | 0,270 s |
| File : enqueue | 0,189 s | 0,045 s |
| File : dequeue | 0,023 s | 0,047 s |
| File : enqueue/dequeue alternés | 0,279 s | 0,277 s |

Interprétation :
- **Pile** : le tableau est 2 à 4 fois plus rapide. Mémoire contiguë (bon usage du cache) et aucun appel à l'allocateur par élément.
- **File, enfilage massif** : le tableau circulaire est plus lent, car chaque doublement recopie tous les éléments ; la liste a un coût constant mais alloue un nœud par élément.
- **Régime stable** : les deux sont équivalents.
- **Complexité** : toutes les opérations sont en O(1) ; le tableau est en O(1) *amorti* (agrandissements rares mais coûteux), la liste en O(1) strict.
- **Conclusion** : pour l'analyseur (expressions courtes) la différence est négligeable. Le tableau est préférable par défaut ; la liste est utile si le temps de chaque opération doit être régulier ou si la taille est très imprévisible.

Les temps dépendent de la machine : relancer `make bench`.

## 10. Mini-projet — Annuaire de contacts

**Structure** : `directory` = tableau de 27 pointeurs vers des `list`. L'indice est `toupper(première lettre) - 'A'` pour A–Z, 26 (`#`) pour tout le reste (chiffres, accents, symboles). Chaque `list` est doublement chaînée (`head`, `tail`, `prev`, `next`), triée par nom sans tenir compte de la casse. Un `node` possède son `contact`.

| Niveau | Constructeur | Destructeur | Affichage |
|---|---|---|---|
| contact | `contact_new` | `contact_free` | `contact_print` |
| nœud | `node_new` | `node_free` | `node_print` |
| liste | `list_new` | `list_free` | `list_print` |
| annuaire | `dir_new` | `dir_free` | `dir_print` |

**Opérations de l'annuaire** :

| Fonction | Retour |
|---|---|
| `dir_add(d, nom, tel)` | `DIR_OK`, `DIR_DUPLICATE` (nom déjà présent, sans casse), `DIR_ERR_ARG`, `DIR_ERR_MEM` |
| `dir_delete(d, nom)` | `DIR_OK` ou `DIR_NOT_FOUND` |
| `dir_search(d, nom)` | `const contact *` ou `NULL` |
| `dir_update(d, nom, nouveau_tel)` | `DIR_OK` ou `DIR_NOT_FOUND` |
| `dir_print(d, fichier)` | affiche les entrées non vides |

La recherche ne parcourt que la liste de la bonne lettre, et s'arrête dès qu'un nom plus grand est rencontré (liste triée).

**Lien avec la deque chaînée** : une deque (file à double entrée) s'implémente naturellement avec une liste doublement chaînée munie de `head` et `tail`. `list_push_front`, `list_push_back`, `list_pop_front` et `list_pop_back` sont en O(1) ; `detach` retire n'importe quel nœud en O(1) grâce à `prev`/`next` (c'est ce qui rend `dir_delete` simple), et `list_insert_sorted` réutilise ces mêmes liens.

## 11. Limites connues

- L'expression est limitée par la taille du tampon de `calc` (512 caractères) ; la bibliothèque `expr.c` elle-même n'a pas de limite fixe, sauf 63 caractères par nombre.
- Les nombres sont des `double` : pas de notation scientifique (`1e5`) ni de constantes (`pi`).
- Pas de multiplication implicite (`2x` est refusé).
- L'extension « deque / fenêtre glissante » n'est pas traitée dans l'analyseur.
