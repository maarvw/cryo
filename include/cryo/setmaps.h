#pragma once

#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iostream> //löschen wenn kein debug mehr nötig
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
using arena = fe::Arena;

namespace cryo {

template<typename T, typename V = void>
class setmaps {

    //custom pair so we can compare T to pair<T,V>
    template<typename A, typename B>
    struct pair {
        A first;
        B second;
        bool operator==(A a) { return first==a; }
        bool operator<(A a) { return first<a;}
        bool operator>(A a) { return first>a;}
        bool operator<=(A a) { return first<=a;}
        bool operator>=(A a) { return first>=a;}

        bool operator==(pair<A,B> o) { return first==o.first; }
        bool operator<(pair<A,B> o) { return first<o.first;}
        bool operator>(pair<A,B> o) { return first>o.first;}
        bool operator<=(pair<A,B> o) { return first<=o.first;}
        bool operator>=(pair<A,B> o) { return first>=o.first;}

    };
    fe::Arena arena_;
    using value_type = std::conditional_t<std::is_void_v<V>, T, pair<T, V>>;

public:
    class setmap { //make it a binary tree for now
    using arena = fe::Arena;

    const bool is_set = std::is_void_v<V>;

    private:
        class node {
        public:
            node(value_type val) :
                is_red_(true), val_(val) {
            }

            node(node* l, node* r, value_type val) :
                is_red_(true), left_(l), right_(r), val_(val) {
                }

            node(node* l, node* r, node* p, value_type val) :
                is_red_(true), left_(l), right_(r), parent_(p), val_(val) {
            }

            ~node() {
               if (has_left())  left_->~node();
               if (has_right()) right_->~node();
            }

            value_type& val() { return val_; }


            node* left() const { return left_ ; }
            node* right() const { return right_; }
            node* parent() const { return parent_; }

            void set_left(node* n) { left_ = n; }
            void set_right(node* n) { right_ = n; }
            void set_parent(node* n) { parent_ = n; }

            bool has_left() { return left_!=nullptr; }
            bool has_right() { return right_!=nullptr; }
            bool is_root() { return parent_==nullptr; }

            bool is_right() { return !is_root() && parent()->right()==this; }
            bool is_left() { return !is_root() && parent()->left()==this; }

            bool is_red() { return is_red_; }
            bool is_black() { return !is_red(); }

            void set_red() { is_red_=true; }
            void set_black() { is_red_=false; }

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
                if (is_root()) return nullptr;
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
                if (is_root()) return nullptr;
                if (cur->is_right()) return cur->parent();
                while (cur->is_left()) cur=cur->parent();
                cur=cur->parent();
                return cur;
            }

        private:

            bool is_red_;
            node* left_ = nullptr;
            node* right_ = nullptr;
            node* parent_ = nullptr;
            value_type val_;
        };

        //red/black balancing stuff -------------------------------------------

        void left_rotate(node* n) {
            node* r = n->right();
            n->set_right(r->left());
            if (r->has_left()) {
                r->left()->set_parent(n);
            }
            r->set_parent(n->parent());
            if (n->is_root()) {
                root_ = r;
            }
            else if (n->is_left()) {
                n->parent()->set_left(r);
            }
            else {
                n->parent()->set_right(r);
            }
            r->set_left(n);
            n->set_parent(r);
        }

        void rightRotate(node* n) {
            node* l = n->left();
            n->set_left(l->right());
            if (l->has_right()) {
                l->right()->set_parent(n);
            }
            l->set_parent(n->parent());
            if (n->is_root()) {
                root_ = l;
            }
            else if (n->is_right()) {
                n->parent()->set_right(l);
            }
            else {
                n->parent()->set_left(l);
            }
            l->set_right(n);
            n->set_parent(l);
        }

        void fix_insert(node* n) { //breaks everything
            return;
            if (n==nullptr) return;
            if (n->parent()==nullptr) return;
            if (n->parent()->parent()==nullptr) return;
            while (n != root_ && n->parent()->is_red()) {
                if (n->parent()->is_left()) {
                    node* u = n->parent()->parent()->right();
                    if (u!=nullptr&&u->is_red()) {
                        n->parent()->set_black();
                        u->set_black();
                        n->parent()->parent()->set_red();
                        n = n->parent()->parent();
                    }
                    else {
                        if (n->is_right()) {
                            n = n->parent();
                            left_rotate(n);
                        }
                        n->parent()->set_black();
                        n->parent()->parent()->set_red();
                        rightRotate(n->parent()->parent());
                    }
                }
                else {
                    node* u = n->parent()->parent()->left();
                    if (u!=nullptr&&u->is_red()) {
                        n->parent()->set_black();
                        u->set_black();
                        n->parent()->parent()->set_red();
                        n = n->parent()->parent();
                    }
                    else {
                        if (n->is_left()) {
                            n = n->parent();
                            rightRotate(n);
                        }
                        n->parent()->set_black();
                        n->parent()->parent()->set_red();
                        left_rotate(n->parent()->parent());
                    }
                }
            }
            root_->set_black();
        }

        //normal class stuff ----------------------------------------------

        node* root_;

        arena* arena_;

        size_t size_;

        setmap() = delete;

        setmap(arena* arena, node* root, size_t size) :
            root_(root), arena_(arena), size_(size) {}


        //private helper functions (both modes)---------------------------

        node* insert_helper(node* n, value_type elem) {
            if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);
            if (n->val()==elem) {
                if (is_set) throw std::runtime_error("set already contains element");
                node* newnode = new (arena_->allocate<node>(1)) node(elem);
                newnode->set_left(n->left());
                newnode->set_right(n->right());
                return newnode;
            }

            node* newnode;
            if (elem>n->val())  newnode = insert_helper(n->right(), elem);
            else                newnode = insert_helper(n->left(), elem);

            node* ret;
            if (elem>n->val())  ret = new (arena_->allocate<node>(1)) node(n->left(), newnode, n->val());
            else                ret = new (arena_->allocate<node>(1)) node(newnode,n->right(), n->val());

            newnode->set_parent(ret);
            return ret;
        }

        bool contains_helper(node* n, T elem) const {
            if (n==nullptr)     return false;
            if (n->val()==elem) return true;
            if (n->val()>elem)  return contains_helper(n->left(), elem);
            else                return contains_helper(n->right(), elem);
        }

        node* find_helper(node* cur, T elem) {
            if (cur==nullptr)     return nullptr;
            if (cur->val()==elem) return cur;
            if (cur->val()<elem)  return find_helper(cur->right(), elem);
            else                  return find_helper(cur->left(), elem);
        }

        //both modes
        void insert_primitive(value_type elem) {
            if (is_set && contains(elem)) {
                if (is_set) throw std::runtime_error("element already exists here");
                else size_--;
            }
            size_++;
            if (root_==nullptr) {
                root_ = new (arena_->allocate<node>(1)) node(elem);
                root_->set_black();
                return;
            }
            node* cur = root_;
            node* prev=nullptr;
            while (cur!=nullptr) {
                prev=cur;
                if (cur->val()<elem) cur=cur->right();
                else if (cur->val()>elem) cur=cur->left();
                else throw std::runtime_error("set insert_primitive failed");
            }
            node* newnode = new (arena_->allocate<node>(1)) node(elem);
            if (prev->val()<elem) prev->set_right(newnode);
            else prev->set_left(newnode);
            newnode->set_parent(prev);
            if constexpr (std::is_void_v<V>) fix_insert(find_helper(root_, elem));
            if constexpr (!std::is_void_v<V>) fix_insert(find_helper(root_, elem.first));
           std::cout<<"cur depth: "<<checkmaxdepth()<<std::endl;

        }

    public:

        ~setmap() {
            if (root_) root_->~node();
        }

        setmap(arena* arena) :
        root_(nullptr), arena_(arena), size_(0) {}

        setmap(value_type elem, arena* arena) :
        root_(nullptr), arena_(arena), size_(0) {
            insert_primitive(elem);
        }

        setmap(std::initializer_list<value_type> elems, arena* arena) :
        root_(nullptr), arena_(arena), size_(0) {
            for (T elem : elems)
                insert_primitive(elem);
        }

        class iterator {
            public:
            using difference_type = std::ptrdiff_t;
            using value_type = setmaps::value_type;
            using iterator_category = std::bidirectional_iterator_tag;

            setmap* setmap_;
            node* current_;

            iterator() = default;

            iterator(setmap* set, node* n) :
            setmap_(set), current_(n) {}

            const value_type& operator*() const {return current_->val(); }

            iterator& operator++() { current_=current_->next(); return *this; }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }

            iterator& operator--() { current_=current_->prev(); return *this; }
            iterator operator--(int) { iterator ret = *this; --(*this); return ret; }

            bool operator==(const iterator& other) const { return setmap_==other.setmap_ && ((current_==nullptr&&other.current_==nullptr) || (current_!=nullptr&&other.current_!=nullptr && this->current_->val() == other.current_->val())); }
            bool operator!=(const iterator& other) const { return !(*this==other); }

            bool operator<(const iterator& other) const { return this->current_->val()<other->current_->val(); };
            bool operator>(const iterator& other) const { return this->current_->val()>other->current_->val(); };
            bool operator<=(const iterator& other) const { return this->current_->val()<=other->current_->val(); };
            bool operator>=(const iterator& other) const { return this->current_->val()>=other->current_->val(); };
        };
        iterator begin() { return iterator(this, root_->leftmost()); }
        iterator end() { return iterator(this, nullptr); }

        class reverse_iterator {
            public:
            using difference_type = std::ptrdiff_t;
            using value_type = setmaps::value_type;
            using iterator_category = std::bidirectional_iterator_tag;

            setmap* setmap_;
            node* current;

            reverse_iterator() = default;

            reverse_iterator(setmap* set, node* n) :
                setmap_(set), current(n) {}

            const value_type& operator*() const {return current->val(); }

            reverse_iterator& operator++() { current=current->prev(); return *this; }
            reverse_iterator operator++(int) { reverse_iterator ret = *this; ++(*this); return ret; }

            reverse_iterator& operator--() { current=current->next(); return *this; }
            reverse_iterator operator--(int) { reverse_iterator ret = *this; --(*this); return ret; }

            bool operator==(const reverse_iterator& other) const { return setmap_==other.setmap_ && ((current==nullptr&&other.current==nullptr) || (current!=nullptr&&other.current!=nullptr && this->current->val() == other.current->val())); }
            bool operator!=(const reverse_iterator& other) const { return !(*this==other); }

            bool operator<(const reverse_iterator& other) const { return this->current->val()>other->current->val(); };
            bool operator>(const reverse_iterator& other) const { return this->current->val()<other->current->val(); };
            bool operator<=(const reverse_iterator& other) const { return this->current->val()>=other->current->val(); };
            bool operator>=(const reverse_iterator& other) const { return this->current->val()<=other->current->val(); };
        };
        reverse_iterator rbegin() { return reverse_iterator(this, root_->rightmost()); }
        reverse_iterator rend() { return reverse_iterator(this, nullptr); }

        size_t size() const { return size_; }

        bool contains(T elem) const { return contains_helper(root_, elem); }

        /*returns an iterator to the node containing elem.
          returns end() if elem isn't in the set.*/
        iterator find(T elem) { return iterator(this, find_helper(root_, elem)); }

        bool operator==(setmap other) const {
            if (size()!=other.size()) return false;
            for (auto elem : other) if (!contains(elem)) return false;
            return true;
        }

        bool operator!=(setmap other) const { return !(*this==other); }


        //set specific functions------------------------------------------

        template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
        setmap* insert(T elem) {
            if (contains(elem)) return this;
            if (root_==nullptr) {
                node* newnode = new (arena_->allocate<node>(1)) node(elem);
                newnode->set_black();
                setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, 1);
                return newset;
            }
            node* newnode = insert_helper(root_, elem);
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,size_+1);
            newset->fix_insert(newset->find_helper(root_, elem));
            std::cout<<"cur depth: "<<newset->checkmaxdepth()<<std::endl;
            return newset;
        }

        template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
        setmap* insert(std::initializer_list<T> elems) {
            setmap* newset = insert(*elems.begin());
            for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem)
                newset->insert_primitive(*elem);
            return newset;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
        bool contains(value_type elem) {
            return contains_helper(root_, elem.first);
        }

        //map specific functions------------------------------------------

        template<typename U = V>
        auto operator[](T key) -> std::enable_if_t<!std::is_void_v<U>, const V>{
            auto kek = find_helper(root_, key);
            if (kek==nullptr) throw std::runtime_error("key does not exist here");
            return kek->val().second;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
        setmap* insert(T key, U value) {
            if (root_==nullptr) {
                node* newnode = new (arena_->allocate<node>(1)) node({key,value});
                setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, 1);
                return newmap;
            }
            size_t newsize = size_+!contains(key); //cursed
            node* newnode = insert_helper(root_, {key,value});
            setmap* newmap =new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,newsize);
            newmap->fix_insert(newmap->find_helper(root_, key));
            return newmap;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
        setmap* insert(std::initializer_list<pair<T,V>> elems) {
            auto bg = *elems.begin();
            setmap* newmap = insert(bg.first,bg.second);
            for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem)
                newmap->insert_primitive(*elem);
            return newmap;
        }

        int depthchecker(node* n){
            if (n==nullptr) return 0;
            return 1+(std::max(depthchecker(n->left()),depthchecker(n->right())));
        }

        int checkmaxdepth() {
            return depthchecker(root_);
        }

        void printer(node* n) {
            if (n==nullptr) return;
            std::cout<<n->val()<<(n->is_black()?"b":"r")<<std::endl<<"-> "<<
            (n->has_left()?std::to_string(n->left()->val()):"X")<<" "<<
            (n->has_right()?std::to_string(n->right()->val()):"X")<<std::endl<<std::endl;
            printer(n->left());
            printer(n->right());
        }

        void printtree() {
            printer(root_);
        }

    };

    setmap initial_setmap;

    setmaps() :
        initial_setmap(setmap(&arena_)) {}

    setmaps(value_type elem) :
        initial_setmap(setmap(elem, &arena_)) {}

    setmaps(std::initializer_list<value_type> elems) :
        initial_setmap(elems, &arena_) {}

    setmap* get() {
        return &initial_setmap;
    }


static_assert(std::bidirectional_iterator<typename setmap::iterator>);
static_assert(std::bidirectional_iterator<typename setmap::reverse_iterator>);
};

}
