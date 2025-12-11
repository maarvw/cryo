#pragma once
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
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
            node(bool is_leaf, node* parent)
             : is_leaf_(is_leaf), parent_(parent) {
                if (!std::is_trivially_default_constructible_v<T> && is_leaf) {
                    for (size_t i = 0; i < M; ++i)
                        new (&leaves_[i]) T();
                }
            }

            ~node() {
                if (!std::is_trivially_destructible_v<T> && is_leaf()) {
                    for (size_t i = 0; i < M; ++i)
                        leaves_[i]->~T();
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

            node* child(size_t i) { return children()[i]; }

            void set_parent(node* p) { parent_ = p; }

        private:
            const bool is_leaf_;

            node* parent_;

            union {
                node* children_[M];
                T leaves_[M];
            };
        };

        /*root node of the tree, unique to every tree / persistent "copy"*/
        node* root_;
        /*numer of elements in the tree*/
        size_t size_;
        /*current capacity of the tree. multiple of M. if full, add() creates new layer*/
        size_t capacity_;
        /*bitshift amount at the root level. power of 2.*/
        size_t shift_;
        /*pointer to the arena of the "parent" trees element to be used for allocation*/
        fe::Arena* arena_;

        tree() = delete;

    public:
        /*default constructor for empty tree*/
        tree(fe::Arena* arena) :
            arena_(arena) {
            empty_init();
        }

        /*single element constructor*/
        tree(T elem, fe::Arena* arena) :
            arena_(arena) {
            empty_init();
            add_primitive(elem);
        }

        /*constructor for initializer list*/
        tree(std::initializer_list<T> elems, fe::Arena* arena) :
            arena_(arena) {
            empty_init();
            for (T elem : elems)
                add_primitive(elem);
        }

        /*adds new element to the tree, expanding the tree if necessary.
        returns a persistent "copy" of the previous tree*/
        tree add(T elem) {
            if (size_+1 >= capacity_) { //tree full, need to expand depth
                int s = shift_;
                node* newroot = new (arena_->allocate<node>(1)) node(false, nullptr);
                newroot->children()[0]=root_;
                newroot->children()[1]= new (arena_->allocate<node>(1)) node(root_->is_leaf(),newroot);
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena_->allocate<node>(1)) node(s!=0, newnode);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                return tree(newroot, size_+1, shift_+B, capacity_*M, arena_);
            }
            node* newroot = insert_at_index(elem, root_, shift_, size_);
            return tree(newroot, size_+1, shift_, capacity_, arena_);
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
            if (i>=size_ || i<0)
                throw std::runtime_error("invalid index");
            node* newroot = insert_at_index(elem, root_, shift_, i);
            return tree(newroot, size_, shift_, capacity_, arena_);
        }

        /*returns element at specific index, making no changes.
        equivalent to the [] operator*/
        const T& get(size_t i) const {
            if (i>=size_ || i<0)
                throw std::runtime_error("invalid index");
            node* cur = root_;
            int s = shift_;
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
        size_t size() const {
            return size_;
        }

        class iterator {
        public:
            tree* tree_;
            size_t idx_;
            node* leaf_;
            iterator(tree* tree, size_t idx = 0) 
             : tree_(tree), idx_(idx), leaf_(tree_->get_leaf(idx_)) {}


            void update_leaf(size_t newidx) {
                size_t diff = newidx-idx_;
                if (!((idx_%M)+diff>=0&&(idx_%M)+diff<M)) leaf_=tree_->get_leaf(idx_);
                idx_=newidx;
            }

            const T& operator*() { return leaf_->leaf(idx_%M); }

            iterator& operator++() { update_leaf(idx_+1); return *this; }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }
            iterator& operator--() { update_leaf(idx_-1); return *this; }
            iterator operator--(int) { iterator ret = *this; --(*this); return ret; }
            
            iterator& operator+=(int n) { update_leaf(idx_+n); return *this; }
            iterator& operator-=(int n) { update_leaf(idx_-n); return *this; }
            
            iterator operator+(int n) { return iterator(tree_, idx_+n); }
            iterator operator-(int n) { return iterator(tree_, idx_-n); }

            bool operator==(iterator other) { return this->idx_==other.idx_ && this->tree_==other.tree_; }
            bool operator!=(iterator other) { return !(*this==other); }
        };
        iterator begin() { return iterator(this); }
        iterator end() { return iterator(this, size_); }

    private:
        /*constructor for new tree, only intended to be used by update functions*/
        tree(node* root, size_t size, size_t shift, size_t capacity, fe::Arena* arena) :
            root_(root), size_(size), shift_(shift), capacity_(capacity), arena_(arena) {}

        /*initialises empty tree to be used by constructors*/
        void empty_init() {
            size_ = 0;
            root_ = new (arena_->allocate<node>(1)) node(true, nullptr);
            shift_ = 0; //bei erhöhung der höhe +B
            capacity_ = M;
        }

        /*adds element in non-persistent way, not intended to be used outside of constructors*/
        void add_primitive(T elem) {
            size_++;
            if (size_>=capacity_) { //new root
                int s = shift_;
                node* newroot = new (arena_->allocate<node>(1)) node(false, nullptr);
                newroot->children()[0]=root_;
                newroot->children()[1]= new (arena_->allocate<node>(1)) node(root_->is_leaf(), newroot);
                node* newnode = newroot->children()[1]; //newroot remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[0] = new (arena_->allocate<node>(1)) node(s==0, newnode);
                    newnode=newnode->children()[0];
                }
                newnode->leaf(0)=elem;
                root_ = newroot;
                shift_+=B;
                capacity_*=M;
                return;
            }
            node* cur = root_;
            int i = size_-1;
            int s = shift_;
            while (s != 0) {
                s -= B;
                if (cur->children()[(i>>s)%M]==nullptr) {
                    cur->children()[(i>>s)%M] = new (arena_->allocate<node>(1)) node(s==0, cur);
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
                node* newleaf = new (arena_->allocate<node>(1)) node(true, nullptr);
                std::copy_n(cur->leaves(), M, newleaf->leaves());
                newleaf->leaves()[i%M]=elem;
                return newleaf;
            }
            node* newinner = new (arena_->allocate<node>(1)) node(false, nullptr);
            std::copy_n(cur->children(), M, newinner->children());
            if (cur->children()[(i>>s)%M]==nullptr) { //need to build new subtree of inners
                node* newnode = newinner; //newinner remains "root" of new subtree
                while (s!=0) {
                    s -= B;
                    newnode->children()[(i>>s)%M] = new (arena_->allocate<node>(1)) node(s==0, newnode);
                    newnode=newnode->children()[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                }
                newnode->leaf(0)=elem;
                return newinner;
            }
            newinner->children()[(i>>s)%M]=insert_at_index(elem, cur->children()[(i>>s)%M], s-B, i);
            newinner->children()[(i>>s)%M]->set_parent(newinner);
            return newinner;
        }

        node* get_leaf(size_t i){
            node* cur = root_;
            int s=shift_;
            while (!cur->is_leaf()){
                cur=cur->child((i>>s)%M);
                s-=B;
            }
            return cur;
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
