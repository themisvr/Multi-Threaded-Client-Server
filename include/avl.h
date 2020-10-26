#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stdint.h>
#include <sys/types.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct avl_tree_node {
	ssize_t height;
	struct avl_tree_node *left;
	struct avl_tree_node *right;
	void *data;
} avl_tree_node;


typedef void *(*avl_data_alloc) (void *avl_data);
typedef void (*avl_data_dealloc) (void *avl_data);
typedef ssize_t (*avl_data_cmp) (void *data_a, void *data_b);


typedef struct avl_tree {
	struct avl_tree_node *root;
	avl_data_cmp data_cmp;
	avl_data_alloc data_alloc;
	avl_data_dealloc data_dealloc;
} avl_tree;


avl_tree *avl_init(avl_data_alloc data_alloc, avl_data_dealloc data_dealloc, avl_data_cmp data_cmp);

avl_tree_node *avl_data_search(avl_tree *avltree, void *avl_data);

size_t avl_size(avl_tree *avltree);

void avl_data_insert(avl_tree *avltree, void *avl_data);

void avl_data_delete(avl_tree *avltree, void *avl_data);

void avl_free(avl_tree *avltree);

int8_t avl_is_balanced(avl_tree_node *avl_node);


#endif // AVL_TREE_H