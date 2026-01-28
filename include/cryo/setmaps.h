#pragma once

#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iostream> //löschen wenn kein debug mehr nötig
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
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
               val_(val) {
            }

            node(node* l, node* r, value_type val) :
                left_(l), right_(r), val_(val) {
                    recalculate_balance();
                }

            ~node() {
               if (has_left())  left_->~node();
               if (has_right()) right_->~node();
            }

            value_type& val() { return val_; }


            node* left() const { return left_ ; }
            node* right() const { return right_; }

            int height() const { return height_; }
            int balance() const { return balance_; }

            void set_left(node* n)  { left_  = n; recalculate_balance(); }
            void set_right(node* n) { right_ = n; recalculate_balance(); }
            void set_val(value_type v) { val_ = v; }

            void recalculate_balance() {
                int rh = (right()!=nullptr?right()->height():0);
                int lh = (left()!=nullptr?left()->height():0);
                height_=1+std::max(lh,rh);
                balance_=rh-lh;
            }
            bool has_left() { return left_!=nullptr; }
            bool has_right() { return right_!=nullptr; }

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

        private:

            node* left_ = nullptr;
            node* right_ = nullptr;
            value_type val_;
            int height_ = 1;
            int balance_ = 0;
        };


        //balancing stuff -------------------------------------------

        /* left and right rotate should ONLY be called during the insertion process for nodes where the 
          right/left nodes respectively should just have been created and are not in use by any previous
          states of the tree so as not to break the traversal for those other trees */

        node* left_rotate(node* n) {
            node* r = n->right();  //
            node* rl = r->left();
            n->set_right(rl); n->recalculate_balance();
            r->set_left(n); r->recalculate_balance();
            return r;
        }

        node* right_rotate(node* n) {
            node* l = n->left();  //
            node* lr = l->right();
            n->set_left(lr); n->recalculate_balance();
            l->set_right(n); l->recalculate_balance();
            return l;
        }

        node* balance_node(node* n) {
            int bal = n->balance();
            if (bal<-1) {
                return right_rotate(n);
            }
            else if (bal>1) {
                return left_rotate(n);
            }
            return n;
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
                //map case: insert new value for existing key in the middle of the tree
               return new (arena_->allocate<node>(1)) node(n->left(), n->right(), elem);
            }

            if (elem>n->val())    return balance_node(new (arena_->allocate<node>(1)) node(n->left(), insert_helper(n->right(), elem), n->val()));
            else                  return balance_node(new (arena_->allocate<node>(1)) node(insert_helper(n->left(), elem), n->right(), n->val()));
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
           std::cout<<"(set_prim) cur depth: "<<checkmaxdepth()<<" "<<root_->height()<<std::endl;
        }

        node* change_or_copy(node* n, std::set<node*>* changed) {
            if (changed->contains(n)) return n;
            return new (arena_->allocate<node>(1)) node(n->left(), n->right(), n->val());
        }

        node* insert_single(node* n, value_type val, std::set<node*>* changed) {
            node* newnode;
            if (n==nullptr) {
                size_++;
                newnode = new (arena_->allocate<node>(1)) node(val);
                changed->insert(newnode);
                return newnode;
            } 

            if (n->val()==val) {
                size_--;
                if (is_set) throw std::runtime_error("set already contains element");
                //map case: insert new value for existing key in the middle of the tree
                newnode = change_or_copy(n, changed);
                newnode->set_val(val);
                //newnode = new (arena_->allocate<node>(1)) node(n->left(), n->right(), val);
                changed->insert(newnode);
                return newnode;
            }
            
            if (val>n->val()) {
                newnode = change_or_copy(n, changed);
                newnode->set_left(n->left());
                newnode->set_right(insert_single(n->right(), val, changed));
                //newnode = new (arena_->allocate<node>(1)) node(n->left(), insert_single(n->right(), val), n->val(),changed);            
            }
            else {
                newnode = change_or_copy(n, changed);
                newnode->set_left(insert_single(n->left(), val, changed));
                newnode->set_right(n->right());
                //newnode = new (arena_->allocate<node>(1)) node(insert_single(n->left(), val), n->right(), n->val(),changed);
            } 
            
            changed->insert(newnode);
            return balance_node(newnode);            
        }

        void insert_list(std::initializer_list<value_type> vals) {
            std::set<node*> changed = std::set<node*>(); //cursed hier auch std::sets zu nutzen aber naja
            
            for (auto v : vals) {
                if (is_set&&contains(v)) continue;
                root_=insert_single(root_, v, &changed);
            }
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
            insert_list(elems);
        }

        struct iterator {
            using difference_type = std::ptrdiff_t;
            using value_type = setmaps::value_type;
            using iterator_category = std::forward_iterator_tag;

            setmap* setmap_;
            node* current_;
            std::vector<node*> stack_ = std::vector<node*>();

            iterator() = default;

            iterator(setmap* set) :
                setmap_(set), current_(set->root_) {
                    if (!current_->has_left()) {
                        return;
                    }
                    stack_.push_back(current_);
                    while (current_->has_left()) {
                        current_ = current_->left();
                        stack_.push_back(current_);
                    }
                    stack_.pop_back();
                }

            iterator(setmap* set, node* n) :
                setmap_(set), current_(set->root_) {
                    if (n==nullptr) {current_=nullptr; return;} //end() case
                    value_type nval = n->val();
                    if (!set->contains(nval)) throw std::runtime_error("bad node");
                    current_ = set->root_;
                    if (current_==n) return;
                    while (current_!=n) {
                        if (current_==nullptr) throw std::runtime_error("bad node");
                        stack_.push_back(current_);
                        if (current_->val()<nval) current_=current_->right();
                        else if (current_->val()>nval) current_=current_->left();
                    }
                }

            iterator(setmap* set, T elem) :
                setmap_(set), current_(set->root_) {
                    if (!set->contains(elem)) throw std::runtime_error("bad node");
                    if (current_->val()==elem) return;
                    while (current_->val()!=elem) {
                        if (current_->val()<elem) current_=current_->right();
                        else if (current_->val()>elem) {
                            stack_.push_back(current_);
                            current_=current_->left();
                        }
                        if (current_==nullptr) throw std::runtime_error("bad node");
                    }
                }

            const value_type& operator*() const {return current_->val(); }

            iterator& operator++() {
                if (current_->has_right()) {
                    current_=current_->right();
                    stack_.push_back(current_);
                    while (current_->has_left()) {
                        current_ = current_->left();
                        stack_.push_back(current_);
                    }
                    stack_.pop_back();
                    return *this;
                }
                if (stack_.empty()) { current_=nullptr; return *this; } //end reached
                current_=stack_.back();
                stack_.pop_back();
                return *this; 
            }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }

            bool operator==(const iterator& other) const { return setmap_==other.setmap_ && ((current_==nullptr&&other.current_==nullptr) || (current_!=nullptr&&other.current_!=nullptr && this->current_->val() == other.current_->val())); }
            bool operator!=(const iterator& other) const { return !(*this==other); }

            bool operator<(const iterator& other) const { return this->current_->val()<other->current_->val(); };
            bool operator>(const iterator& other) const { return this->current_->val()>other->current_->val(); };
            bool operator<=(const iterator& other) const { return this->current_->val()<=other->current_->val(); };
            bool operator>=(const iterator& other) const { return this->current_->val()>=other->current_->val(); };
        };
        iterator begin() { return iterator(this); }
        iterator end() { return iterator(this, nullptr); }

        size_t size() const { return size_; }

        bool contains(T elem) const { return contains_helper(root_, elem); }

        /*returns an iterator to the node containing elem.
          returns end() if elem isn't in the set.*/
        iterator find(T elem) { return iterator(this, elem); }

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
                setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, 1);
                return newset;
            }
            node* newnode = insert_helper(root_, elem);
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,size_+1);
            std::cout<<"cur depth: "<<newset->checkmaxdepth()<<" "<<newnode->height()<<std::endl;
            return newset;
        }

        template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
        setmap* insert(std::initializer_list<T> elems) {
            //contains_all check?
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,size_);
            newset->insert_list(elems);
            // setmap* newset = insert(*elems.begin());
            // for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem)
            //     newset->insert_primitive(*elem);
            return newset;
        }

        //map specific functions------------------------------------------

        template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
        bool contains(value_type elem) {
            return contains_helper(root_, elem.first);
        }

        template<typename U = V,  typename = std::enable_if_t<!std::is_void_v<U>>>
        const V operator[](T key){
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
            return newmap;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
        setmap* insert(std::initializer_list<pair<T,V>> elems) {
            setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,size_);
            newmap->insert_list(elems);
            // setmap* newmap = insert(bg.first,bg.second);
            // for (auto elem = std::next(elems.begin(), 1); elem != elems.end(); ++elem)
            //     newmap->insert_primitive(*elem);
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
            std::cout<<"val: "<<n->val()<<", height: "<<n->height()<<std::endl<<"-> "<<
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


static_assert(std::forward_iterator<typename setmap::iterator>);
};

}
