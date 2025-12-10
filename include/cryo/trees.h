#pragma once
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include "../../external/fe/include/fe/arena.h"
namespace cryo {

/*radix balanced tree with persistent functionality,
  serves as underlying datastructure for containers.
  represents a "family" of trees*/
template<typename T, size_t B = 5>
class trees {
public:
    /*amount of array elements per node (power of 2)*/
    static const size_t M = 1 << B;
    /*singular tree within the family with
    individual root element*/
    class tree {
    private:
        /*node of the rbtree*/
        class node {
        public:
            node(bool is_leaf)
             : is_leaf_(is_leaf) {
                if (is_leaf) {
                    for (size_t i = 0; i < M; ++i) {
                        new (&leaves_[i]) T();  //necessary for strings to not cause segfault
                    }
                }
                }

            bool is_leaf() const { return is_leaf_; }
            bool is_inner() const { return !is_leaf(); }

            T* leaves() {
                assert(is_leaf());
                return leaves_;
            }

            T& leaf(size_t i) { return leaves()[i]; }

            node** children() {
                assert(is_inner());
                return children_;
            }

            node* child(size_t i) const { return children()[i]; }

        private:
            const bool is_leaf_;

            union {
                node* children_[M];
                T leaves_[M];
            };
        };

        /*root node of the tree, unique to every tree / persistent "copy"*/
        node* root;
        /*numer of elements in the tree*/
        size_t size;
        /*current capacity of the tree. multiple of M. if full, add() creates new layer*/
        size_t capacity;
        /*bitshift amount at the root level. power of 2.*/
        size_t shift;
        /*id of the "parent" trees class, technically redundant with arena*/
        int trees_id;
        /*pointer to the arena of the "parent" trees element to be used for allocation*/
        fe::Arena* arena;

        tree() = delete;
    public:
        /*default constructor for empty tree*/
        tree(fe::Arena* arena) :
            arena(arena) {
            empty_init();
        }

        /*single element constructor*/
        tree(T elem, fe::Arena* arena) :
            arena(arena) {
            empty_init();
            add_primitive(elem);
        }

        /*constructor for initializer list*/
        tree(std::initializer_list<T> elems, fe::Arena* arena) :
            arena(arena) {
            empty_init();
            for (T elem : elems)
                add_primitive(elem);
        }

        /*adds new element to the tree, expanding the tree if necessary.
        returns a persistent "copy" of the previous tree*/
        tree add(T elem) {
            if (size+1 >= capacity) { //tree full, need to expand depth
                int s = shift;
                node* newroot = new (arena->allocate<node>(1)) node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new (arena->allocate<node>(1)) node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena->allocate<node>(1)) node(s!=0);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                return tree(newroot, size+1, shift+B, capacity*M, arena);
            }
            node* newroot = insert_at_index(elem, root, shift, size);
            return tree(newroot, size+1, shift, capacity, arena);
        }

        /*adds whole list of elements at once, using add_primitive to prevent unnecessary allocation*/
        tree add(std::initializer_list<T> elems) {
            tree newtree = add(*elems.begin());
            for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem) {
                newtree.add_primitive(*elem);
            }
            return newtree;
        }

        /*inserts an element at a specific (previously existing) index in the tree,
        overwriting the data at that index. returns a persistent "copy" of the previous tree.*/
        tree insert(size_t i, T elem) {
            if (i>=size || i<0)
                throw std::runtime_error("invalid index");
            node* newroot = insert_at_index(elem, root, shift, i);
            return tree(newroot, size, shift, capacity, arena);
        }

        /*returns element at specific index, making no changes.
        equivalent to the [] operator*/
        const T& get(size_t i) const {
            if (i>=size || i<0)
                throw std::runtime_error("invalid index");
            node* cur = root;
            int s = shift;
            while (s != 0) {
                cur = cur->children()[(i >> s) % M];
                s -= B;
            }
            return cur->leaf(i%M);
        }

        /*returns element at specific index, making no changes.
        equivalent to the get() function*/
        const T& operator[] (size_t i) const {
            return get(i);
        }

        /*returns number of elements in the tree*/
        size_t getSize() const {
            return size;
        }

    private:
        /*constructor for new tree, only intended to be used by update functions*/
        tree(node* root, size_t size, size_t shift, size_t capacity, fe::Arena* arena) :
            root(root), size(size), shift(shift), capacity(capacity), arena(arena) {}

        /*initialises empty tree to be used by constructors*/
        void empty_init() {
            size = 0;
            root = new (arena->allocate<node>(1)) node(true);
            shift = 0; //bei erhöhung der höhe +B
            capacity = M;
        }

        /*adds element in non-persistent way, not intended to be used outside of constructors*/
        void add_primitive(T elem) {
            size++;
            if (size>=capacity) { //new root
                int s = shift;
                node* newroot = new (arena->allocate<node>(1)) node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new (arena->allocate<node>(1)) node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena->allocate<node>(1)) node(s==0);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                root = newroot;
                shift+=B;
                capacity*=M;
                return;
            }
            node* cur = root;
            int i = size-1;
            int s = shift;
            while (s != 0) {
                s -= B;
                if (cur->children()[(i>>s)%M]==nullptr) {
                    cur->children()[(i>>s)%M] = new (arena->allocate<node>(1)) node(s==0);
                }
                cur = cur->children()[(i >> s) % M];
            }
            cur->leaf(i%M)=elem;
        }

        /*recursive helper function adding an element at a specific index in the tree,
        expanding the tree if necessary. creates new nodes and returns a new root element
        for the new persistent "copy"*/
        node* insert_at_index(T elem, node* cur, int s, size_t i) {
            if (s==0) {
                //copy & change leaf
                node* newleaf = new (arena->allocate<node>(1)) node(true);
                std::copy_n(cur->leaves(), M, newleaf->leaves());
                std::cout<<"copied"<<std::endl;
                newleaf->leaves()[i%M]=elem;
                return newleaf;
            }
            node* newinner = new (arena->allocate<node>(1)) node(false);
            std::uninitialized_copy_n(cur->children(), M, newinner->children());
            if (cur->children()[(i>>s)%M]==NULL) { //need to build new subtree of inners
                node* newnode = newinner; //newinner remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[(i>>s)%M] = new (arena->allocate<node>(1)) node(s==0);
                    newnode=newnode->children()[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                }
                newnode->leaf(0)=elem;
                return newinner;
            }
            newinner->children()[(i>>s)%M]=insert_at_index(elem, cur->children()[(i>>s)%M], s-B, i);
            return newinner;
        }

    };
private:
    /*arena used for the arena allocation*/
    fe::Arena arena;

    /*the initial tree generated from this trees element*/
    tree initial_tree;

public:
    /*default constuctor creating 1 empty tree*/
    trees() :
        initial_tree(tree(&arena)) {}

    /*constructor creating tree with 1 element*/
    trees(T elem) :
        initial_tree(tree(elem, &arena)) {}

    /*constructor creating tree with initial elements*/
    trees(std::initializer_list<T> elems) :
        initial_tree(tree(elems, &arena)) {}

    /*returns the initial tree created from this trees*/
    tree get() {
        return initial_tree;
    }

};

}
