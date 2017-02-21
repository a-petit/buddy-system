#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>

typedef struct bloc bloc;

// --- private ---

// memory_reserve : repère l'adresse du début de la mémoire réserve
extern void * memory_reserve;

// freebloclists implante la liste des listes de bloc disponibles
extern bloc **freebloclists;

// renvoie la liste des blocs disponibles de rang rank
extern bloc * get_list_by_rank(char rank);

// get_bloc_by_rank : renvoie le premier élément
//   de la liste des blocs disponibles de rang r
extern bloc * get_bloc_by_rank(char rank);

// get_bloc_by_subd : renvoie un bloc d'indice de taille rank en subdivisant
//   un bloc de taille rank+1. Si aucun bloc de taille rank+1 n'est disponible,
//   la fonction poursuit sa recherche de façon récursive
extern bloc * get_bloc_by_subd(char rank);

// m_merge : procède à la fusion du bloc p avec son co-bloc quand celui-ci est
//   disponible et poursuit de façon récursive sur le bloc résultant.
void m_merge(bloc * p);

// --- public ---

// m_reserve : initialise la réserve de mémoire.
extern int m_reserve();

// m_dispose : libère la réservecde de mémoire.
extern void m_dispose();

// m_alloc : allocates space for an object whose size is specified by
//   size and whose value is indeterminate. The m_alloc function returns
//   either a null pointer or a pointer to the allocated space.
extern void * m_alloc(size_t size);

// m_free : causes the space pointed to by ptr to be deallocated, that is,
//   made available for further allocation. If ptr is a null pointer,
//   no action occurs. Otherwise, if the argument does not match a pointer
//   earlier returned by a memory management function, or if the space has been
//   deallocated by a call to free or realloc, the behavior is undefined.
extern void m_free(void * ptr);

// m_print : affiche la réserve de mémoire.
extern void m_print();

#endif
