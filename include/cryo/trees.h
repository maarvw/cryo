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

        /*initialises empty tree to be used by constructors*/
        void empty_init() {
            size = 0;
            root = new node(true);
            shift = 0; //bei erhöhung der höhe +B
            capacity = M;
        }

        /*contructor for new tree, only intended to be used by update functions*/
        tree(node* root, size_t size, size_t shift, size_t capacity) :
            root(root), size(size), shift(shift), capacity(capacity) {}

        /*adds element in non-persistent way, not intended to be used outside of constructors*/
        void add_primitive(T elem) {
            size++;
            std::cout<<"new elem, new size="<<size<<std::endl;
            if (size>=capacity) { //new root
                std::cout<<"new cap"<<std::endl;
                int s = shift;
                node* newroot = new node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new node(s==0);
                    newnode=newnode->children()[0];
                }
                newnode->leaf[0]=elem;
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
                    cur->children()[(i>>s)%M] = new node(s==0);
                }
                cur = cur->children()[(i >> s) % M];
            }
            cur->leaf[i%M]=elem;
        }

        /*recursive helper function adding an element at a specific index in the tree,
        expanding the tree if necessary. creates new nodes and returns a new root element
        for the new persistent "copy"*/
        node* insert_at_index(T elem, node* cur, int s, size_t i) {
            if (s==0) {
                std::cout<<"insert base case"<<std::endl;
                //copy & change leaf
                node* newleaf = new node(true);
                std::copy_n(cur->leaves(), M, newleaf->leaves());
                newleaf->leaves()[i%M]=elem;
                return newleaf;
            }
            std::cout<<"add with s="<<s<<std::endl;
            node* newinner = new node(false);
            std::copy_n(cur->children(), M, newinner->children());
            if (cur->children()[(i>>s)%M]==NULL) { //need to build new subtree of inners
                node* newnode = newinner; //newinner remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[(i>>s)%M] = new node(s==0);
                    newnode=newnode->children()[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                }
                newnode->leaf(0)=elem;
                return newinner;
            }
            newinner->children()[(i>>s)%M]=insert_at_index(elem, cur->children()[(i>>s)%M], s-B, i);
            return newinner;
        }


        /*default constructor for empty tree*/
        tree() {
            empty_init();
        }

        /*single element constructor*/
        tree(T elem) {
            empty_init();
            add_primitive(elem);
        }

        /*constructor for initializer list*/
        tree(std::initializer_list<T> elems) {
            empty_init();
            for (T elem : elems)
                add_primitive(elem);
        }

        /*adds new element to the tree, expanding the tree if necessary.
        returns a persistent "copy" of the previous tree*/
        tree add(T elem) {
            if (size+1 >= capacity) { //tree full, need to expand depth
                std::cout<<"add base case"<<std::endl;
                int s = shift;
                node* newroot = new node(false);
                newroot->children()[0]=root;
                newroot->children()[1]= new node(root->is_leaf());
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new node(s!=0);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                return tree(newroot, size+1, shift+B, capacity*M);
            }
            node* newroot = insert_at_index(elem, root, shift, size);
            return tree(newroot, size+1, shift, capacity);
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
            return tree(newroot, size, shift, capacity);
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

    tree latest_tree;

    trees() {
        latest_tree = tree();
    }

    trees(T elem) {
        latest_tree = tree(elem);
    }

    trees(std::initializer_list<T> elems) {
        latest_tree = tree(elems);
    }

    tree add(tree t1, T elem) {
        return latest_tree = t1.add(elem);
    }

    tree insert(tree t1, size_t i, T elem) {
        return latest_tree = t1.insert(i, elem);
    }

    tree get() {
        return latest_tree;
    }


    // trees family = trees();
    // tree tree1 = family.get();
    // tree tree2 = family.add(tree1, 42);
};

}
