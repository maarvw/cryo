#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <type_traits>
using arena = fe::Arena;

namespace cryo {

template<typename T>
class sets {
    fe::Arena arena_;

    class set { //make it a binary tree for now
    using arena = fe::Arena;
    private:
        class node {
            public:

            node(T val) :
                left_(nullptr), right_(nullptr), parent_(nullptr) {
                    if (!std::is_trivially_default_constructible_v<T>)
                        new (&val_) T();
                    val_=val;
                }     

            ~node() { 
                if (std::is_trivially_destructible_v<T>) return;
               // no val_.~T(); necessary, is done automatically at the end, i guess because value not pointer?
               if (has_left())  left_->~node(); 
               if (has_right()) right_->~node(); 
            }

            node(node* l, node* r, T val) :
                left_(l), right_(r), parent_(nullptr) {
                    if (!std::is_trivially_default_constructible_v<T>)
                        new (&val_) T();
                    val_=val;
                }

            node(node* l, node* r, node* p, T val) :
                left_(l), right_(r), parent_(p) {
                    if (!std::is_trivially_default_constructible_v<T>)
                        new (&val_) T();
                    val_=val;
                }

            T& val() { return val_; }
            node* left() const { return left_ ; }
            node* right() const { return right_; }
            node* parent() const { return parent_; }

            void set_left(node* n) { left_ = n; }
            void set_right(node* n) { right_ = n; }
            void set_parent(node* n) { parent_ = n; }

            bool has_left() { return left_!=nullptr; }
            bool has_right() { return right_!=nullptr; }
            bool is_root() { return parent_==nullptr; }

            bool is_right() { return parent()->right()==this; }
            bool is_left() { return parent()->left()==this; }

            node* leftmost() {
                node* n = this;
                while (n->has_left()) n = n->left();
                return n;
            }
            node* rightmost() {
                node* n = this;
                while (n->has_right()) n = n->right();
                return n;
            }

            node* next() {
                node* cur = this;
                if (cur->has_right()) {
                    cur = cur->right();
                    cur = cur->leftmost();
                    return cur;
                }
                if (cur->is_root()) throw std::runtime_error("dummer hurensohn");
                if (cur->is_left()) return cur->parent();
                while (cur->is_right()) cur=cur->parent();
                cur=cur->parent();
                return cur;
            }

            node* prev() {
                node* cur = this;
                if (cur->has_left()) {
                    cur = cur->left();
                    cur = cur->rightmost();
                    return cur;
                }
                if (cur->is_root()) throw std::runtime_error("dummer hurensohn");
                if (cur->is_right()) return cur->parent();
                while (cur->is_left()) cur=cur->parent();
                cur=cur->parent();
                return cur;
            }
            
            private:

            node* left_;
            node* right_;
            node* parent_;
            T val_;
        };

        node* root_;

        arena* arena_;

        size_t size_;      

        set(arena* arena, node* root, size_t size) :
            root_(root), arena_(arena), size_(size) {}

        node* insert_helper(node* n, T elem) {
            if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);
            if (n->val()==elem) throw std::runtime_error("dummer hurensohn");

            node* newnode;
            if (elem>n->val())  newnode = insert_helper(n->right(), elem);
            else                newnode = insert_helper(n->left(), elem);

            node* ret;
            if (elem>n->val())  ret = new (arena_->allocate<node>(1)) node(n->left(), newnode, n->val());
            else                ret = new (arena_->allocate<node>(1)) node(newnode,n->right(), n->val());
            
            newnode->set_parent(ret);
            return ret;
        }

        bool contains_helper(node* n, T elem) {
            if (n==nullptr)     return false;
            if (n->val()==elem) return true;
            if (n->val()>elem)  return contains_helper(n->left(), elem);
            else                return contains_helper(n->right(), elem);
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
                if (elem>cur->val()) cur=cur->right();
                else if (elem<cur->val()) cur=cur->left();
                else throw std::runtime_error("dummer hurensohn");
            }
            node* newnode = new (arena_->allocate<node>(1)) node(elem);
            if (elem>prev->val()) prev->set_right(newnode);
            else prev->set_left(newnode);
            newnode->set_parent(prev);
        }

    public:

        ~set() {
            if (root_==nullptr) return;
            root_->~node();
        }

        size_t size() const { return size_; }

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

        set insert(T elem) {
            assert(!contains(elem)); //assume elem isnt already inserted
            if (root_==nullptr) {
                node* newnode = new (arena_->allocate<node>(1)) node(elem);
                return set(arena_, newnode, 1);
            }
            return set(arena_,insert_helper(root_, elem),size_+1);
        }

        set insert(std::initializer_list<T> elems) {
            set newset = insert(*elems.begin());
            for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem)
                newset.insert_primitive(*elem);
            return newset;
        }

        bool contains(T elem) {
            return contains_helper(root_, elem);
        }

        class iterator {
        public:
            using difference_type = std::ptrdiff_t;
            using value_type = T;

            node* current;

            iterator() = default;

            iterator(node* n) :
                current(n) {}

            const T& operator*() const {return current->val(); }

            iterator& operator++() { current=current->next(); return *this; }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }

            iterator& operator--() { current=current->prev(); return *this; }
            iterator operator--(int) { iterator ret = *this; --(*this); return ret; }

            bool operator==(iterator other) { return this->current->val() == other.current->val(); }
            bool operator!=(iterator other) { return !(*this==other); }

            bool operator<(const iterator& other) const { return this->current->val()<other->current->val(); };
            bool operator>(const iterator& other) const { return this->current->val()>other->current->val(); };
            bool operator<=(const iterator& other) const { return this->current->val()<=other->current->val(); };
            bool operator>=(const iterator& other) const { return this->current->val()>=other->current->val(); };
        };
        iterator begin() { return iterator(root_->leftmost()); }
        iterator end() { return iterator(root_->rightmost()); }

    };

    set initial_set;

public:


    sets() :
        initial_set(set(&arena_)) {}
    
    sets(T elem) :
        initial_set(set(elem, &arena_)) {}

    sets(std::initializer_list<T> elems) :
        initial_set(elems, &arena_) {}

    set get() {
        return initial_set;
    }



};


}