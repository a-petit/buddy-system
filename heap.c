#include "heap.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// -- print utils

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define PRINTN(c, n) for (long unsigned i = 0; i < n; ++i) { fputc(c, stdout); }


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -- bloc ---------------------------------------------------------------------

typedef struct bloc bloc;

typedef struct bloc {
  char rank;
  bool available;
  union {
    struct {
      bloc *prev;
      bloc *next;
    } chains;     // when available = true
    char data;    // when available = false
  } u;
} bloc;

// donne la taille de l'espace occupé par les informations d'en-tête
// du bloc telles que le compteur de taille et l'indicateur de service
#define BLOC_INFOS_SIZE (sizeof(char) + sizeof(bool))

// donne la taille d'allocation minimale pour une donnée.
#define BLOC_DATAS_SIZE_MIN (sizeof(bloc) - BLOC_INFOS_SIZE) // <<<<<<<<<<<<<<<< [!] data ne commence pas nécessairement juste après available ie p+BLOC_INFOS_SIZE ??

// --- getters / setters

// bloc_get_rank : renvoie le rang du bloc pointé par p
char bloc_get_rank(const bloc *p) {
  return p->rank;
}

// bloc_set_rank : affecte au bloc pointé par p l'indice de rang rank
void bloc_set_rank(bloc *p, char rank) {
  p->rank = rank;
}

// bloc_is_available : renvoie true si le bloc pointé par p est disponible
bool bloc_is_available(const bloc *p) {
  return p->available;
}

// bloc_set_available : affecte au bloc pointé par p l'indicateur de service b
void bloc_set_available(bloc *p, bool b) {
  p->available = b;
}

// --- list features

// bloc_empty : génère une fausse tête de liste
bloc * bloc_empty(void) {
  bloc * p = (bloc *) malloc(sizeof(bloc));
  p->available = true;
  p->u.chains.prev = NULL;
  p->u.chains.next = NULL;
  return p;
}

// bloc_free : libère la fausse tête de liste
// [!] Ici, ne libère en aucun cas les blocs éventuellements chainée à p
void bloc_free(bloc * p) {
  free(p);
}

// bloc_get_next : renvoie l'adresse du bloc chainé au bloc p dans le cas où p
//   repère un bloc disponible, un pointeur nul sinon
bloc * bloc_get_next(const bloc * p) {
  return p->available ? p->u.chains.next : NULL;
}

// bloc_get_prev : renvoie l'adresse du bloc chainé au bloc p dans le cas où p
//   repère un bloc disponible, un pointeur nul sinon
bloc * bloc_get_prev(const bloc * p) {
  return p->available ? p->u.chains.prev : NULL;
}

// bloc_set_next : si p est disponible, affecte q au champ next du bloc p,
//    affiche un message d'alerte sinon
// void bloc_set_next(bloc * p, bloc * x)
// [dangereux : liste doublement châinées]

// bloc_set_next : si p est disponible, affecte q au champ prev du bloc p,
//    affiche un message d'alerte sinon
// void bloc_set_prev(bloc * p, bloc * x);
// [dangereux : liste doublement châinées]

// bloc_insert_head : insère le bloc pointé par p en tête de la liste implanté
//   par la fausse tête s
void bloc_insert_head(bloc *s, bloc *p) {
  p->u.chains.next = s->u.chains.next;
  p->u.chains.prev = s;
  if (s->u.chains.next != NULL) {
    (s->u.chains.next)->u.chains.prev = p;
  }
  s->u.chains.next = p;
}

// bloc_self_remove :
void bloc_self_remove(bloc *x) {
  if (x->u.chains.next != NULL) {
    (x->u.chains.next)->u.chains.prev = x->u.chains.prev;
  }
  if (x->u.chains.prev != NULL) {
    (x->u.chains.prev)->u.chains.next = x->u.chains.next;
  }
  x->u.chains.prev = NULL;
  x->u.chains.next = NULL;
}

// --- data utils

// bloc_get_data_ptr : renvoie l'adresse des données du bloc passé en paramètre.
void * bloc_get_data_ptr(bloc * p) {
  // printf("diff = %lu\n", (char *) &(p->u.data) - (char *) p); // MAGIC #1 //- [!] le décallage est différent de BLOC_INFOS_SIZE ??
  return &(p->u.data);
}

// bloc_from_data_ptr : renvoie l'adresse du bloc relatif
//    aux données dont l'adresse est passée en paramètre
bloc * bloc_from_data_ptr(void * ptr) {
  return (bloc *)((char *) ptr - 8u); // MAGIC #1 ------------------------------ == BLOC_HEAD_SIZE ? Non. pourquoi ?
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -- heap ---------------------------------------------------------------------

// -- constants

#define FUN_FAILURE 1
#define FUN_SUCCESS 0


#define SYSTEM_ORDER_DEFAUT 10

#ifndef SYSTEM_ORDER
#define SYSTEM_ORDER SYSTEM_ORDER_DEFAUT
#endif

#define RANK_MIN (ceil(log2((double) sizeof(bloc))))
#define RANK_MAX (SYSTEM_ORDER)
#define RANK_NBR (RANK_MAX - RANK_MIN + 1)

#define SIZE_BY_RANK(n) ((size_t) pow(2.0, (double) (n)))
#define RANK_BY_SIZE(x) ((char)(ceil(log2((double) (x)))))

#define SYSTEM_SIZE SIZE_BY_RANK(SYSTEM_ORDER)


void * memory_reserve;

bloc **freebloclists;

bloc * get_list_by_rank(char rank) {
  int r = rank - (int) RANK_MIN;
  if (r < 0 || r >= (int) RANK_NBR) {
    fprintf(stderr, "error :: get_list_by_rank %d out of boundaries", r);
  }
  return freebloclists[r];
}

bloc * get_bloc_by_rank(char rank) {
  return bloc_get_next( get_list_by_rank(rank) );
}

bloc * get_bloc_by_subd(char rank) {
  if (rank > RANK_MAX - 1) {
    return NULL;
  }
  bloc * p = get_bloc_by_rank((char) (rank + 1));
  if (p == NULL) {
    p = get_bloc_by_subd((char) (rank + 1)); //--------------------------------- [!] NOT TAIL_REC
  }
  if (p == NULL) {
    return NULL;
  }

  bloc_self_remove(p);

  bloc * s = get_list_by_rank(rank);

  // left co-bloc
  bloc_set_rank(p, rank);
  bloc_insert_head(s, p);

  // right co-bloc
  p = (bloc *) ((char *) p + SIZE_BY_RANK(bloc_get_rank(p)));
  bloc_set_available(p, true);
  bloc_set_rank(p, rank);
  bloc_insert_head(s, p);
  return p;
}

void m_merge(bloc * p) {
  char rank = bloc_get_rank(p);
  size_t pos = (size_t) ((char *) p - (char *) memory_reserve);
  size_t size = SIZE_BY_RANK(rank);

  bloc * q;
  bloc * x;
  if (pos % (2 * size) == 0) {
    q = (bloc *) ((char *) p + size);
    x = p;
  } else {
    q = (bloc *) ((char *) p - size);
    x = q;
  }
  if (bloc_is_available(q) && bloc_get_rank(q) == rank) {
    bloc_self_remove(q);

    bloc_set_rank(x, (char) (rank + 1));
    bloc_insert_head(get_list_by_rank(bloc_get_rank(x)), x);
    m_merge(x);
  }
}


int m_reserve(void) {
  printf("M_RESERVE\n");
  /* debug infos
  printf("SYSTEM_ORDER : %d\n", SYSTEM_ORDER);
  printf("SYSTEM_SIZE : %d\n", (int) SYSTEM_SIZE);
  printf("size of bloc : %lu\n", sizeof(bloc));
  printf("size of head : %lu\n", BLOC_INFOS_SIZE);
  printf("RANK_MIN : %d\n", (int) RANK_MIN);
  printf("RANK_MAX : %d\n", (int) RANK_MAX);
  printf("RANK_NBR : %d\n", (int) RANK_NBR);

  for (size_t i = 0; i < SYSTEM_SIZE; ++i) {
    fputc('-', stdout);
  }
  fputc('\n', stdout);*/

  memory_reserve = malloc(SIZE_BY_RANK( SYSTEM_ORDER ));
  if (memory_reserve == NULL) {
    return FUN_FAILURE;
  }

  freebloclists = (bloc **) malloc(sizeof(bloc *) * (size_t) RANK_NBR);
  if (freebloclists == NULL) {
    free(memory_reserve);
    return FUN_FAILURE;
  }

  bloc **p = freebloclists;
  bloc **e = freebloclists + (size_t) RANK_NBR;
  // IB : les listes repérés par les pointeurs allant de freebloclists à p
  //      ont été créée (c'est à dire allouées et initialisés)
  // QC : e-p
  while (p < e) {
    *p = bloc_empty();
    ++p;
  }

  bloc *x = (bloc *) memory_reserve;
  bloc_set_rank(x, SYSTEM_ORDER);
  bloc_set_available(x, true);
  bloc_insert_head(get_list_by_rank(SYSTEM_ORDER), x);

  return FUN_SUCCESS;
}

void m_dispose(void) {
  printf("M_DISPOSE\n");

  bloc **p = freebloclists;
  bloc **e = freebloclists + (size_t) RANK_NBR;
  // IB : le blocs allant de freebloclists à p (exclus) on été libérés,
  // QC : e-p
  while (p < e) {
    bloc_free(*p);
    ++p;
  }
  free(freebloclists);
  free(memory_reserve);
}

void * m_alloc(size_t s) {
  size_t size = s;
  if (size > SIZE_MAX - BLOC_INFOS_SIZE) {
    printf("M_ALLOC : overflow : type\n");
    return NULL;
  }
  if (size > SYSTEM_SIZE - BLOC_INFOS_SIZE) {
    printf("M_ALLOC : overflow : size %lu exceed memory space\n", size);
    return NULL;
  }
  // adjust to min size
  if (size < BLOC_DATAS_SIZE_MIN) {
    size = BLOC_DATAS_SIZE_MIN;
  }
  // increment by header datas space
  size += BLOC_INFOS_SIZE;

  char rank = RANK_BY_SIZE(size);
  bloc * p = get_bloc_by_rank(rank);
  if (p == NULL) {
    p = get_bloc_by_subd(rank);
  }
  if (p == NULL) {
    return NULL;
  }
  bloc_self_remove(p);
  bloc_set_available(p, false);

  size = SIZE_BY_RANK( bloc_get_rank(p) );

  printf("M_ALLOC : taille portée à %lu (+%lu)\n", size, size - s);
  printf("M_ALLOC : %p\n", (void *) p);

  return bloc_get_data_ptr(p);
}

void m_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }
  bloc * p = bloc_from_data_ptr(ptr);
  printf("M_FREE  : %p\n", (void *) p);

  bloc_set_available(p, true);
  bloc * s = get_list_by_rank(bloc_get_rank(p));
  bloc_insert_head(s, p);

  m_merge(p);
}

// m_print_bloc : affiche le bloc p
void m_print_bloc(const bloc * p) {
  printf(KYEL "%c", '0' + p->rank);
  printf(KNRM "%d", bloc_is_available(p));
  size_t data_size = SIZE_BY_RANK(p->rank) - BLOC_INFOS_SIZE;
  if (bloc_is_available(p)) {
    printf(KGRN);
    int offset = 0;
    offset += printf("%p", (void *)(bloc_get_prev(p)));
    offset += printf("%p", (void *)(bloc_get_next(p)));
    if (offset >= 0 && data_size > (size_t) offset) {
      PRINTN('-', data_size - (size_t) offset);
    }
  } else {
    printf(KBLU);
    char c = *((char *) bloc_get_data_ptr((bloc *) p));
    printf("-%c-", c);
    PRINTN('#', data_size - 3);
  }
  printf(KNRM);
}

void m_print() {
  // printf("M_PRINT\n");
  bloc *p = (bloc *) memory_reserve;
  char *e = (char *) p + SYSTEM_SIZE;
  // IB : les informations comprises entre memory_reserve et p on été affichés
  // QC : e-p
  while ((char *) p < e) {
    m_print_bloc(p);
    p = (bloc *) ((char *) p + SIZE_BY_RANK(p->rank));
  }
  printf("\n");
}
