#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iostream> //löschen wenn kein debug mehr nötig
#include <iterator>
#include <stdexcept>
#include <type_traits>
using arena = fe::Arena;

namespace cryo {

template<typename T, typename V = void>
class setmaps {

    fe::Arena arena_;

public:
    class setmap { //make it a binary tree for now
    using arena = fe::Arena;

    using value_type = std::conditional_t<std::is_same_v<V, void>, T, std::pair<T, V>>;
    
    private:
        class node {
        public:
            node(value_type val) :
                left_(nullptr), right_(nullptr), parent_(nullptr) {
                    if (!std::is_trivially_default_constructible_v<T>)
                        new (&val_) T();
                    val_=val;
                }     

            node(node* l, node* r, value_type val) :
                left_(l), right_(r), parent_(nullptr) {
                    if (!std::is_trivially_default_constructible_v<T>)
                        new (&val_) T();
                    val_=val;
                }

            node(node* l, node* r, node* p, value_type val) :
                left_(l), right_(r), parent_(p) {
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

            node* left_;
            node* right_;
            node* parent_;
            value_type val_;
        };

        node* root_;

        arena* arena_;

        size_t size_;      

        setmap() = delete;

        setmap(arena* arena, node* root, size_t size) :
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

        ~setmap() {
            if (root_==nullptr) return;
            root_->~node();
        }

        size_t size() const { return size_; }

        setmap(arena* arena) : 
            root_(nullptr), arena_(arena), size_(0) {}

        setmap(T elem, arena* arena) : 
            root_(nullptr), arena_(arena), size_(0) {
                insert_primitive(elem);
            }

        setmap(std::initializer_list<T> elems, arena* arena) :
            root_(nullptr), arena_(arena), size_(0) {
                for (T elem : elems)
                    insert_primitive(elem);
            }

        setmap insert(T elem) {
            assert(!contains(elem)); //assume elem isnt already inserted
            if (root_==nullptr) {
                node* newnode = new (arena_->allocate<node>(1)) node(elem);
                return setmap(arena_, newnode, 1);
            }
            return setmap(arena_,insert_helper(root_, elem),size_+1);
        }

        setmap insert(std::initializer_list<T> elems) {
            setmap newset = insert(*elems.begin());
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
            using value_type = setmap::value_type;
            using iterator_category = std::bidirectional_iterator_tag;

            setmap* set_;
            node* current_;

            iterator() = default;

            iterator(setmap* set, node* n) :
                set_(set), current_(n) {}

            const value_type& operator*() const {return current_->val(); }

            iterator& operator++() { current_=current_->next(); return *this; }
            iterator operator++(int) { iterator ret = *this; ++(*this); return ret; }

            iterator& operator--() { current_=current_->prev(); return *this; }
            iterator operator--(int) { iterator ret = *this; --(*this); return ret; }

            bool operator==(const iterator& other) const { return set_==other.set_ && ((current_==nullptr&&other.current_==nullptr) || (current_!=nullptr&&other.current_!=nullptr && this->current_->val() == other.current_->val())); }
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
            using value_type = T;
            using iterator_category = std::bidirectional_iterator_tag;

            setmap* set_;
            node* current;

            reverse_iterator() = default;

            reverse_iterator(setmap* set, node* n) :
                set_(set), current(n) {}

            const T& operator*() const {return current->val(); }

            reverse_iterator& operator++() { current=current->prev(); return *this; }
            reverse_iterator operator++(int) { reverse_iterator ret = *this; ++(*this); return ret; }

            reverse_iterator& operator--() { current=current->next(); return *this; }
            reverse_iterator operator--(int) { reverse_iterator ret = *this; --(*this); return ret; }

            bool operator==(const reverse_iterator& other) const { return set_==other.set_ && ((current==nullptr&&other.current==nullptr) || (current!=nullptr&&other.current!=nullptr && this->current->val() == other.current->val())); }
            bool operator!=(const reverse_iterator& other) const { return !(*this==other); }

            bool operator<(const reverse_iterator& other) const { return this->current->val()>other->current->val(); };
            bool operator>(const reverse_iterator& other) const { return this->current->val()<other->current->val(); };
            bool operator<=(const reverse_iterator& other) const { return this->current->val()>=other->current->val(); };
            bool operator>=(const reverse_iterator& other) const { return this->current->val()<=other->current->val(); };
        };
        reverse_iterator rbegin() { return reverse_iterator(this, root_->rightmost()); }
        reverse_iterator rend() { return reverse_iterator(this, nullptr); }

        /*returns an iterator to the node containing elem.
          returns end() if elem isn't in the set.*/
        iterator find(T elem) {
            return iterator(this, find_helper(root_, elem));
        }

        node* find_helper(node* cur, T elem) {
            if (cur==nullptr) return nullptr;
            if (elem>cur->val()) return find_helper(cur->right(), elem);
            else return find_helper(cur->left(), elem);
        }

        //map stuff
        template<typename U = V>
        auto operator[](T key) -> std::enable_if_t<!std::is_same<U, void>::value, const V>{
            std::cout<<"find"<<std::endl;
            auto kek = find_helper_map(root_, key);
            if (kek==nullptr) std::cout<<"shit"<<std::endl;
            return kek->val().second;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_same<U, void>::value>> 
        node* find_helper_map(node* cur, T key) {
            if (cur==nullptr) return nullptr;
            std::cout<<key<<" "<<cur->val().first<<" >> ";
            if (cur->has_left()) std::cout<<cur->left()->val().first;
            std::cout<<", ";
            if (cur->has_right()) std::cout<<cur->right()->val().first;
            std::cout<<std::endl;
            if (cur->val().first==key) return cur;
            if (key>cur->val().first) return find_helper_map(cur->right(), key);
            else return find_helper_map(cur->left(), key);
        }
    
        template <typename U = V, typename = std::enable_if_t<!std::is_same<U, void>::value>>        
        setmap insert(T key, U value) {
            if (root_==nullptr) {
                node* newnode = new (arena_->allocate<node>(1)) node({key,value});
                return setmap(arena_, newnode, 1);
            }
            return setmap(arena_,insert_helper(root_, {key,value}),size_+1);
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_same<U, void>::value>>
        node* insert_helper(node* n, std::pair<U,V> elem)  {
            if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);
            if (n->val().first==elem.first) { 
                node* newnode = new (arena_->allocate<node>(1)) node(elem);
                newnode->set_left(n->left());
                newnode->set_right(n->right());
                return newnode;
            }

            node* newnode;
            if (elem.first>n->val().first)  newnode = insert_helper(n->right(), elem);
            else                            newnode = insert_helper(n->left(), elem);

            node* ret;
            if (elem.first>n->val().first)  ret = new (arena_->allocate<node>(1)) node(n->left(), newnode, n->val());
            else                            ret = new (arena_->allocate<node>(1)) node(newnode,n->right(), n->val());
            
            newnode->set_parent(ret);
            return ret;
        }

        template <typename U = V, typename = std::enable_if_t<!std::is_same<U, void>::value>>
        void insert_primitive(T key, U value) {
            size_++;
            if (root_==nullptr) {
                root_ = new (arena_->allocate<node>(1)) node({key,value});
                return;
            }
            node* cur = root_;
            node* prev=nullptr;
            while (cur!=nullptr) {
                prev=cur;
                if (key>cur->val().first) cur=cur->right();
                else if (key<cur->val().first) cur=cur->left();
                else throw std::runtime_error("dummer hurensohn");
            }
            node* newnode = new (arena_->allocate<node>(1)) node({key,value});
            if (key>prev->val().first) prev->set_right(newnode);
            else prev->set_left(newnode);
            newnode->set_parent(prev);
        }

    };

    setmap initial_set;

    setmaps() :
        initial_set(setmap(&arena_)) {}
    
    setmaps(T elem) :
        initial_set(setmap(elem, &arena_)) {}

    setmaps(std::initializer_list<T> elems) :
        initial_set(elems, &arena_) {}

    setmap get() {
        return initial_set;
    }


static_assert(std::bidirectional_iterator<typename setmap::iterator>);
static_assert(std::bidirectional_iterator<typename setmap::reverse_iterator>);
};

}