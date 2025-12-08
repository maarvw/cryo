#pragma once
#include <algorithm>
#include <initializer_list>
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
    
    /*id of the trees class, every tree generated from here will have the same*/
    int id;
    /*counter for tracking ids*/
    inline static int counter = 0;
   

    /*arena used for the arena allocation*/
    fe::Arena arena;

    /*singular tree within the family with
    individual root element*/
    class tree {
    public:
        /*node of the rbtree*/
        class node {
        public:
            /*default constructor using arena allocators*/
            node(bool is_leaf)
                : is_leaf_(is_leaf) {}

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

        /*initialises empty tree to be used by constructors*/
        void empty_init() {
            size = 0;
            root = new (arena) node(true);
            shift = 0; //bei erhöhung der höhe +B
            capacity = M;
        }

        /*contructor for new tree, only intended to be used by update functions*/
        tree(node* root, size_t size, size_t shift, size_t capacity, int id) :
            root(root), size(size), shift(shift), capacity(capacity), trees_id(id) {}

        /*adds element in non-persistent way, not intended to be used outside of constructors*/
        void add_primitive(T elem) {
            size++;
            //std::cout<<"new elem, new size="<<size<<std::endl;
            if (size>=capacity) { //new root
                //std::cout<<"new cap"<<std::endl;
                int s = shift;
                node* newroot = new (arena) node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new (arena) node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena) node(s==0);
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
                    cur->children()[(i>>s)%M] = new (arena) node(s==0);
                }
                cur = cur->children()[(i >> s) % M];
            }
            cur->leaf(i%M)=elem;
        }

        /*recursive helper function adding an element at a specific index in the tree,
        expanding the tree if necessary. creates new (arena) nodes and returns a new root element
        for the new persistent "copy"*/
        node* insert_at_index(T elem, node* cur, int s, size_t i) {
            if (s==0) {
                //copy & change leaf
                node* newleaf = new (arena) node(true);
                std::copy_n(cur->leaves(), M, newleaf->leaves());
                newleaf->leaves()[i%M]=elem;
                return newleaf;
            }
            //std::cout<<"add with s="<<s<<std::endl;
            node* newinner = new (arena) node(false);
            std::copy_n(cur->children(), M, newinner->children());
            if (cur->children()[(i>>s)%M]==NULL) { //need to build new subtree of inners
                node* newnode = newinner; //newinner remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[(i>>s)%M] = new (arena) node(s==0);
                    newnode=newnode->children()[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                }
                newnode->leaf(0)=elem;
                return newinner;
            }
            newinner->children()[(i>>s)%M]=insert_at_index(elem, cur->children()[(i>>s)%M], s-B, i);
            return newinner;
        }

        tree() = delete;

        /*default constructor for empty tree*/
        tree(int id, fe::Arena* arena) :
            arena(arena), trees_id(id) {
            empty_init();
        }

        /*single element constructor*/
        tree(T elem, int id, fe::Arena* arena) :
            arena(arena), trees_id(id) {
            empty_init();
            add_primitive(elem);
        }

        /*constructor for initializer list*/
        tree(std::initializer_list<T> elems, int id, fe::Arena* arena) :
            arena(arena), trees_id(id) {
            empty_init();
            for (T elem : elems)
                add_primitive(elem);
        }

        /*adds new element to the tree, expanding the tree if necessary.
        returns a persistent "copy" of the previous tree*/
        tree add(T elem) {
            if (size+1 >= capacity) { //tree full, need to expand depth
                //std::cout<<"add base case"<<std::endl;
                int s = shift;
                node* newroot = new (arena) node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new (arena) node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena) node(s!=0);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                return tree(newroot, size+1, shift+B, capacity*M, trees_id);
            }
            node* newroot = insert_at_index(elem, root, shift, size);
            return tree(newroot, size+1, shift, capacity, trees_id);
        }

        /*adds whole list of elements at once but no fancy transience stuff (yet)*/
        tree add(std::initializer_list<T> elems) {
            tree newtree;
            for (T elem : elems) {
                newtree = newtree.add(elem);
            }
            return newtree;
        }

        /*inserts an element at a specific (previously existing) index in the tree,
        overwriting the data at that index. returns a persistent "copy" of the previous tree.*/
        tree insert(size_t i, T elem) {
            if (i>=size || i<0)
                throw std::runtime_error("invalid index");
            node* newroot = insert_at_index(elem, root, shift, i);
            return tree(newroot, size, shift, capacity, trees_id);
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

    };

    /*the last tree generated from this trees element*/
    tree latest_tree;

    /*default constuctor creating 1 empty tree*/
    trees() :
        latest_tree(tree(id=++counter, &arena)) {}

    /*constructor creating tree with 1 element*/
    trees(T elem) :
        latest_tree(tree(elem, id=++counter, &arena)) {}

    /*constructor creating tree with initial elements*/
    trees(std::initializer_list<T> elems) :
        latest_tree(tree(elems, id=++counter, &arena)) {}

    /*adds a new element to the tree by creating a persistent copy.
      throws an exception if the tree did not come from this trees*/
    tree add(tree t1, T elem) {
        if (t1.trees_id!=id)
            throw std::runtime_error("tree id does not match trees id");
        return latest_tree = t1.add(elem);
    }
    /*inserts a new element at an index of the tree by creating a persistent copy.
      throws an exception if the tree did not come from this trees*/
    tree insert(tree t1, size_t i, T elem) {
        if (t1.trees_id!=id)
            throw std::runtime_error("tree id does not match trees id");
        return latest_tree = t1.insert(i, elem);
    }

    /*returns the last tree created from this trees*/
    tree get() {
        return latest_tree;
    }

};

}
