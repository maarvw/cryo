#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <stdexcept>
using arena = fe::Arena;

namespace cryo {

template<typename T>
class sets {
    fe::Arena arena_;

    class set { //make it a binary tree for now
    using arena = fe::Arena;
    public:
        class node {
            public:
            node* left_;
            node* right_;
            T val_;

            node(node* l, node* r, T val) :
                left_(l), right_(r), val_(val) {}

            node(T val) :
                left_(nullptr), right_(nullptr), val_(val) {}
        };

        node* root_;

        arena* arena_;

        size_t size_;

        size_t size() {
            return size_;
        }

        set(arena* arena) : 
            root_(nullptr), arena_(arena), size_(0) {}

        set(T elem, arena* arena) : 
            root_(nullptr), arena_(arena), size_(0) {
                insert_primitive(elem);
            }

        set(std::initializer_list<T> elems, arena* arena) :
            root_(nullptr), arena_(arena), size_(0) {
                for (T elem : elems)
                    insert_primitive(elem);
            }

        set(arena* arena, node* root, size_t size) :
            root_(root), arena_(arena), size_(size) {}

        set insert(T elem) {
            assert(!contains(elem)); //assume elem isnt already inserted
            if (root_==nullptr) {
                node newnode = node(elem);
                return set(arena_, &newnode, 1);
            }
            return set(arena_,insert_helper(root_, elem),size_+1);
        }

        node* insert_helper(node* n, T elem) {
            if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);
            if (n->val_==elem) throw std::runtime_error("dummer hurensohn");
            if (elem>n->val_) return new (arena_->allocate<node>(1)) node(n->left_, insert_helper(n->right_, elem), n->val_);
            else return new (arena_->allocate<node>(1)) node(insert_helper(n->left_, elem),n->right_, n->val_);
        }

        void insert_primitive(T elem) {
            assert(!contains(elem)); //assume elem isnt already inserted
            size_++;
            if (root_==nullptr) {
                root_ = new (arena_->allocate<node>(1)) node(elem);
                return;
            }
            node* cur = root_;
            node* prev=nullptr;
            while (cur!=nullptr) {
                prev=cur;
                if (elem>cur->val_) cur=cur->right_;
                else if (elem<cur->val_) cur=cur->left_;
                else throw std::runtime_error("dummer hurensohn");
            }
            node* newnode = new (arena_->allocate<node>(1)) node(elem);
            (elem>prev->val_ ? prev->right_ : prev->left_) = newnode;
        }

        bool contains(T elem) {
            return contains_helper(root_, elem);
        }

        bool contains_helper(node* n, T elem) {
            if (n==nullptr) return false;
            if (n->val_==elem) return true;
            if (n->val_>elem) return contains_helper(n->left_, elem);
            else return contains_helper(n->right_, elem);
        }

    };

public:
    set initial_set;

    sets() :
        initial_set(set(&arena_)) {}
    
    sets(T elem) :
        initial_set(set(elem, &arena_)) {}

    set get() {
        return initial_set;
    }

};


}