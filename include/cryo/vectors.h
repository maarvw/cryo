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
using arena = fe::Arena;
namespace cryo {
/*vector implementation using radix balanced tree with 
  persistent functionality, represents a "family" of vectors*/
template<typename T, size_t B = 5>
class vectors {

    /*arena used for the arena allocation*/
    arena arena_;
    /*amount of array elements per node (power of 2)*/
    static const size_t M = 1 << B;    

public:
    /*singular tree within the family with
    individual root element*/
    class vector {
    using arena = fe::Arena;

    private:
        /*node of the rbtree*/
        class node {
        public:
            node(bool is_leaf)
             : is_leaf_(is_leaf) {
                if (is_leaf) {
                    if (!std::is_trivially_default_constructible_v<T> && is_leaf) {
                        for (size_t i = 0; i < M; ++i)
                            new (&leaves_[i]) T();
                    }
                }
                else {
                    for (size_t i = 0; i < M; ++i)
                        children_[i] = nullptr; //without this: misaligned memory error after exactly 81952 inserts
                }
            }

            ~node() {
                if (std::is_trivially_destructible_v<T>) return; //we only want this destructor for strings and such
                if (!is_leaf_){
                    for (auto child : children_) { if (child==nullptr) break; delete child; }
                }
                else {
                    for (size_t i = 0; i < M; ++i){
                        leaves_[i].~T();
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

            node*& child(size_t i) { return children()[i]; }

        private:
            const bool is_leaf_;

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
        arena* arena_;

        vector() = delete;
        /*constructor for new tree, only intended to be used by update functions*/
        vector(node* root, size_t size, size_t shift, size_t capacity, arena* arena) :
            root_(root), size_(size), shift_(shift), capacity_(capacity), arena_(arena) {}

        /*initialises empty tree to be used by constructors*/
        void empty_init() {
            size_ = 0;
            root_ = new (arena_->allocate<node>(1)) node(true);
            shift_ = 0; //bei erhöhung der höhe +B
            capacity_ = M;
        }

        /*adds element in non-persistent way, not intended to be used outside of constructors*/
        void add_primitive(T elem) {
            size_++;
            if (size_>capacity_) { //new root
                int s = shift_;
                node* newroot = new (arena_->allocate<node>(1)) node(false);
                newroot->child(0)=root_;
                newroot->child(1)= new (arena_->allocate<node>(1)) node(root_->is_leaf());
                node* newnode = newroot->child(1); //newroot remains "root" of new subtree
                while (s!=0) {
                    newnode->child(0) = new (arena_->allocate<node>(1)) node(s==B);
                    newnode=newnode->child(0);
                    s -= B;
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
                if (cur->child((i>>s)%M)==nullptr) {
                    cur->child((i>>s)%M) = new (arena_->allocate<node>(1)) node(s==0);
                }
                cur = cur->child((i>>s)%M);
            }
            cur->leaf(i%M)=elem;
        }

        /*recursive helper function adding an element at a specific index in the tree,
        expanding the tree if necessary. creates new nodes and returns a new root element
        for the new persistent "copy"*/
        node* insert_at_index(T elem, node* cur, int s, size_t i) {
            if (s==0) {
                //copy & change leaf
                node* newleaf = new (arena_->allocate<node>(1)) node(true);
                std::copy_n(cur->leaves(), M, newleaf->leaves()); 
                newleaf->leaf(i%M)=elem;
                return newleaf;
            }
            node* newinner = new (arena_->allocate<node>(1)) node(false);
            std::copy_n(cur->children(), M, newinner->children());
            if (cur->child((i>>s)%M)==nullptr) { //need to build new subtree of inners
                node* newnode = newinner; //newinner remains "root" of new subtree
                while (s!=0) {
                    newnode->child((i>>s)%M) = new (arena_->allocate<node>(1)) node(s==B);
                    newnode=newnode->child((i>>s)%M); //(i>>s)%M should always be 0 in this loop but just in case
                    s -= B;
                }
                newnode->leaf(0)=elem;
                return newinner;
            }
            newinner->child((i>>s)%M)=insert_at_index(elem, cur->child((i>>s)%M), s-B, i);
            return newinner;
        }

        node* get_leaf(size_t i) const {
            node* cur = root_;
            int s=shift_;
            while (!cur->is_leaf()){
                cur=cur->child((i>>s)%M);
                s-=B;
            }
            return cur;
        }

    public:
        /*default constructor for empty tree*/
        vector(arena* arena) :
            arena_(arena) {
            empty_init();
        }

        /*single element constructor*/
        vector(T elem, arena* arena) :
            arena_(arena) {
            empty_init();
            add_primitive(elem);
        }

        /*constructor for initializer list*/
        vector(std::initializer_list<T> elems, arena* arena) :
            arena_(arena) {
            empty_init();
            for (T elem : elems)
                add_primitive(elem);
        }

        ~vector(){
            root_->~node();
        }

        /*adds new element to the tree, expanding the tree if necessary.
        returns a persistent "copy" of the previous tree*/
        vector push_back(T elem) {
            if (size_+1 > capacity_) { //tree full, need to expand depth
                int s = shift_;
                node* newroot = new (arena_->allocate<node>(1)) node(false);
                newroot->child(0)=root_;
                newroot->child(1)= new (arena_->allocate<node>(1)) node(root_->is_leaf());
                node* newnode = newroot->child(1); //newroot remains "root" of new subtree
                while (s!=0) {
                    newnode->child(0) = new (arena_->allocate<node>(1)) node(s==B);
                    newnode=newnode->child(0);
                    s -= B;
                }
                newnode->leaf(0)=elem;
                return vector(newroot, size_+1, shift_+B, capacity_*M, arena_);
            }
            node* newroot = insert_at_index(elem, root_, shift_, size_);
            return vector(newroot, size_+1, shift_, capacity_, arena_);
        }

        /*adds whole list of elements at once, using add_primitive to prevent unnecessary allocation*/
        vector push_back(std::initializer_list<T> elems) {
            vector newtree = push_back(*elems.begin());
            for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem) {
                newtree.add_primitive(*elem);
            }
            return newtree;
        }

        /*inserts an element at a specific (previously existing) index in the tree,
        overwriting the data at that index. returns a persistent "copy" of the previous tree.*/
        vector insert(size_t i, T elem) {
            if (i>=size_ || i<0)
                throw std::runtime_error("invalid index");
            node* newroot = insert_at_index(elem, root_, shift_, i);
            return vector(newroot, size_, shift_, capacity_, arena_);
        }

        /*returns element at specific index, making no changes.
        equivalent to the [] operator*/
        const T& get(size_t i) const {
            if (i>=size_ || i<0)
                throw std::runtime_error("invalid index");
            node* cur = root_;
            int s = shift_;
            while (s != 0) {
                cur = cur->child((i >> s) % M);
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
            using difference_type = std::ptrdiff_t;   
            using value_type = T;                    
            using reference = const T&;                     
            using pointer = T*;             
            using iterator_category = std::random_access_iterator_tag;

            vector* vec_;
            size_t idx_;
            node* leaf_;

            iterator() = default;

            iterator(vector* tree, size_t idx = 0) 
             : vec_(tree), idx_(idx), leaf_(vec_->get_leaf(idx_)) {}


            void update_leaf(size_t newidx) {
                size_t diff = newidx-idx_;
                if (!((idx_%M)+diff>=0&&(idx_%M)+diff<M)) leaf_=vec_->get_leaf(newidx);
                idx_=newidx;
            }

            const T& operator*() const { return leaf_->leaf(idx_%M); }
            reference operator[](difference_type n) const { return *(this+n); }

            iterator& operator++() { update_leaf(idx_+1); return *this; }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }
            iterator& operator--() { update_leaf(idx_-1); return *this; }
            iterator operator--(int) { iterator ret = *this; --(*this); return ret; }
            
            iterator& operator+=(difference_type n) { update_leaf(idx_+n); return *this; }
            iterator& operator-=(difference_type n) { update_leaf(idx_-n); return *this; }
            
            iterator operator+(difference_type n) const { return iterator(vec_, idx_+n); }
            iterator operator-(difference_type n) const { return iterator(vec_, idx_-n); }

            bool operator==(iterator other) const { return this->idx_==other.idx_ && this->vec_==other.vec_; }
            bool operator!=(iterator other) const { return !(*this==other); }

            bool operator<(const iterator& other) const { return idx_<other.idx_; };
            bool operator>(const iterator& other) const { return idx_>other.idx_; };
            bool operator<=(const iterator& other) const { return idx_<=other.idx_; };
            bool operator>=(const iterator& other) const { return idx_>=other.idx_; };

            difference_type operator-(iterator other) const { return idx_-other.idx_; }
            difference_type operator+(iterator other) const { return idx_+other.idx_; }

            friend iterator operator+(difference_type n, const iterator& it) { return it+n; }
            friend iterator operator-(difference_type n, const iterator& it) { return it-n; }
        };
        iterator begin() { return iterator(this); }
        iterator end() { return iterator(this, size_); }

        class reverse_iterator {
        public:
            using difference_type = std::ptrdiff_t;   
            using value_type = T;                    
            using reference = const T&;                     
            using pointer = T*;             
            using iterator_category = std::random_access_iterator_tag;

            vector* vec_;
            size_t idx_;
            node* leaf_;

            reverse_iterator() = default;

            reverse_iterator(vector* tree, size_t idx) 
             : vec_(tree), idx_(idx), leaf_(vec_->get_leaf(idx_)) {}


            void update_leaf(size_t newidx) {
                size_t diff = newidx-idx_;
                if (!((idx_%M)-diff>=0&&(idx_%M)-diff<M)) leaf_=vec_->get_leaf(newidx);
                idx_=newidx;
            }

            const T& operator*() const { return leaf_->leaf(idx_%M); }
            reference operator[](difference_type n) const { return *(this+n); }

            reverse_iterator& operator++() { update_leaf(idx_-1); return *this; }
            reverse_iterator operator++(int) { reverse_iterator ret = *this; ++(*this); return ret; }
            reverse_iterator& operator--() { update_leaf(idx_+1); return *this; }
            reverse_iterator operator--(int) { reverse_iterator ret = *this; --(*this); return ret; }
            
            reverse_iterator& operator+=(difference_type n) { update_leaf(idx_-n); return *this; }
            reverse_iterator& operator-=(difference_type n) { update_leaf(idx_+n); return *this; }
            
            reverse_iterator operator+(difference_type n) const { return reverse_iterator(vec_, idx_-n); }
            reverse_iterator operator-(difference_type n) const { return reverse_iterator(vec_, idx_+n); }

            bool operator==(reverse_iterator other) const { return this->idx_==other.idx_ && this->vec_==other.vec_; }
            bool operator!=(reverse_iterator other) const { return !(*this==other); }

            bool operator<(const reverse_iterator& other) const { return idx_>other.idx_; };
            bool operator>(const reverse_iterator& other) const { return idx_<other.idx_; };
            bool operator<=(const reverse_iterator& other) const { return idx_>=other.idx_; };
            bool operator>=(const reverse_iterator& other) const { return idx_<=other.idx_; };

            difference_type operator-(reverse_iterator other) const { return idx_+other.idx_; }
            difference_type operator+(reverse_iterator other) const { return idx_-other.idx_; }

            friend reverse_iterator operator+(difference_type n, const reverse_iterator& it) { return it-n; }
            friend reverse_iterator operator-(difference_type n, const reverse_iterator& it) { return it+n; }
        };
        reverse_iterator rbegin() { return reverse_iterator(this, size_-1); }
        reverse_iterator rend() { return reverse_iterator(this, -1); }

        /*creates a new tree with the current elements in reverse order*/
        vector reverse() {
            vector newtree = vector(arena_);
            reverse_iterator ri = rbegin();
            while (ri != rend()) {
                newtree.add_primitive(*ri);
                ri++;
            }
            return newtree;
        }

    };
    
    /*the first vector generated at initialization*/
    vector initial_vector;

    /*default constuctor creating 1 empty vector*/
    vectors() :
        initial_vector(vector(&arena_)) {}

    /*constructor creating vector with 1 element*/
    vectors(T elem) :
        initial_vector(vector(elem, &arena_)) {}

    /*constructor creating vector with initial elements*/
    vectors(std::initializer_list<T> elems) :
        initial_vector(vector(elems, &arena_)) {}

    /*returns the initial vector created from these vectors*/
    vector get() const {
        return initial_vector;
    }

//asserts to check if the iterators fit all criteria for being random access iterators
static_assert(std::random_access_iterator<typename vector::iterator>);
static_assert(std::random_access_iterator<typename vector::reverse_iterator>);
};

}
