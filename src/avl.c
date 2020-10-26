#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "avl.h"


static inline avl_tree_node *avl_node_create(void *avl_data, avl_data_alloc data_alloc) {
	avl_tree_node *avl_node = malloc(sizeof(avl_tree_node));
	if (!avl_node) {
		fprintf(stderr, "Error: Could not allocate memory for avl_tree_node");
		exit(EXIT_FAILURE);
	}
	avl_node->left = NULL;
	avl_node->right = NULL;
	avl_node->height = 0;
	avl_node->data = data_alloc(avl_data);
	return avl_node;
}


avl_tree *avl_init(avl_data_alloc data_alloc, avl_data_dealloc data_dealloc, avl_data_cmp data_cmp) {
	assert(data_alloc || data_cmp);
	avl_tree *avltree = malloc(sizeof(avl_tree));
	if (!avltree) {
		fprintf(stderr, "Error: Could not allocate memory for avl_tree\n");
		exit(EXIT_FAILURE);
	}
	avltree->root = NULL;
	avltree->data_cmp = data_cmp;
	avltree->data_alloc = data_alloc;
	avltree->data_dealloc = data_dealloc;
	return avltree;
}


avl_tree_node *avl_data_search(avl_tree *avltree, void *avl_data) {	
	assert(avltree || avl_data);
	avl_tree_node *avl_node = avltree->root;
	while (avl_node) {
		ssize_t compare_res = avltree->data_cmp(avl_node->data, avl_data);
		if (compare_res > 0) {
			avl_node = avl_node->left;
		}
		else if (compare_res < 0) {
			avl_node = avl_node->right;
		}
		else { return avl_node; }
	}
	return NULL;
}


static size_t avl_size_rec(avl_tree_node *node) {   
    if(!node) return 0;
    return avl_size_rec(node->left) + avl_size_rec(node->right) + 1;
}


size_t avl_size(avl_tree *avltree) {  
    assert(avltree);
    size_t nodes_count = avl_size_rec(avltree->root);
    return nodes_count;
}


static avl_tree_node *avl_rotate_right(avl_tree_node *avl_node) {
	avl_tree_node *new_root = avl_node->right;
	avl_node->right = new_root->left;
	new_root->left = avl_node;
	return new_root;
}


static avl_tree_node *avl_rotate_left(avl_tree_node *avl_node) {	
	avl_tree_node *new_root = avl_node->left;
	avl_node->left = new_root->right;
	new_root->right = avl_node;
	return new_root;
}


static avl_tree_node *avl_rotate_left_right(avl_tree_node *avl_node) {	
	avl_tree_node *new_root = avl_node->left->right;
	avl_tree_node *tmp_node = avl_node->left;
	avl_node->left = new_root->right;
	tmp_node->right = new_root->left;
	new_root->left = tmp_node;
	new_root->right = avl_node;
	return new_root;
}


static avl_tree_node *avl_rotate_right_left(avl_tree_node *avl_node) {	
	avl_tree_node *tmp_node = avl_node->right;
    avl_tree_node *new_root = avl_node->right->left;
    avl_node->right = new_root->left;
    tmp_node->left = new_root->right;
    new_root->right = tmp_node;
    new_root->left = avl_node;
    return new_root;
}


static inline int avl_node_height(avl_tree_node *avl_node) {	
	if (!avl_node) { return 0; }
	return avl_node->height;
}


static inline int avl_node_bf(avl_tree_node *avl_node) {	
	if (!avl_node) { return 0; }
	return (avl_node_height(avl_node->left) - avl_node_height(avl_node->right));
}


static avl_tree_node *avl_balance_node(avl_tree_node *avl_node) {
	avl_node->height = 1 + MAX(avl_node_height(avl_node->left), avl_node_height(avl_node->right));
	ssize_t balance_factor = avl_node_bf(avl_node);
	if (balance_factor == 2) {
		if (avl_node_bf(avl_node->left) <= -1) {
			return avl_rotate_left_right(avl_node);
		}
        else {
        	return avl_rotate_left(avl_node);
        }
	}
	else if (balance_factor == -2) {
		if (avl_node_bf(avl_node->right) >= 1) {
			return avl_rotate_right_left(avl_node);
		}
		else {
			return avl_rotate_right(avl_node);
		}
	}
	return avl_node;
}


static avl_tree_node *avl_insert_rec(avl_tree_node *avl_node, void *avl_data, avl_data_cmp data_cmp, avl_data_alloc data_alloc) {	
	if (!avl_node) {
		return avl_node_create(avl_data, data_alloc);
	}
	ssize_t compare_res = data_cmp(avl_node->data, avl_data);
	if (compare_res > 0) {
		avl_node->left = avl_insert_rec(avl_node->left, avl_data, data_cmp, data_alloc);
	}
	else if (compare_res <= 0) {
		avl_node->right = avl_insert_rec(avl_node->right, avl_data, data_cmp, data_alloc);
	}
	return avl_balance_node(avl_node);
}


void avl_data_insert(avl_tree *avltree, void *avl_data) {	
	assert(avltree || avl_data);
	avltree->root = avl_insert_rec(avltree->root, avl_data, avltree->data_cmp, avltree->data_alloc);
}


int8_t avl_is_balanced(avl_tree_node* avl_node) {     
    ssize_t lheight, rheight;
    if (!avl_node) { return 1; } 
    lheight = avl_node_height(avl_node->left); 
    rheight = avl_node_height(avl_node->right);
    if (abs(lheight - rheight) <= 1 && avl_is_balanced(avl_node->left) && avl_is_balanced(avl_node->right)) {
        return 1; 
    }
    return 0; 
} 


void avl_data_delete(avl_tree *avltree, void *avl_data) {
	assert(avltree || avl_data);	
}


static void avl_free_rec(avl_tree *avltree, avl_tree_node *avl_node) {	
	if (avl_node) {
		avl_free_rec(avltree, avl_node->left);
		avl_free_rec(avltree, avl_node->right);
		avltree->data_dealloc(avl_node->data);
		free(avl_node);
	}
}


void avl_free(avl_tree *avltree) {
	avl_free_rec(avltree, avltree->root);
	free(avltree);
}