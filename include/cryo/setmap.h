#pragma once

#include "../../external/fe/include/fe/arena.h"
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <fstream>

using arena = fe::Arena;

#define ARR_LIMIT 16

namespace cryo {

class already_inserted_exception : public std::exception {};


/*persistent immutable set/map implementation using self-balancing binary trees.
  represents a set if v==void and a map otherwise.*/
template<typename T, typename V = void, typename Compare = std::less<T>>
class setmap {
    public:
    /*for internal compatibility between sets & maps*/
    using value_type = std::conditional_t<std::is_void_v<V>, T, std::pair<T, V>>;
    using mapped_type = V;

    using arena = fe::Arena;

    /*bool representing whether this is a set or a map*/
    static constexpr bool is_set = std::is_void_v<V>;

    private:
    /*singular node of the tree. includes self-balancing functionality
        but does not feature parent nodes.*/
    class node {
        public:
        /*constructor setting only the value, mostly for leaf nodes*/
        node(value_type val) :
            val_(val) {
        }

        /*constructor for inner nodes. recalculates height and balance.*/
        node(node* l, node* r, value_type val) :
            left_(l), right_(r), val_(val) {
                recalculate_balance();
            }

        /*destructor traversing the child nodes, necessary for non-trivial types*/
        ~node() {
            //if (has_left())  left_->~node();
            //if (has_right()) right_->~node();
        }

        value_type& val() { return val_; }

        T& key() {
            if constexpr (is_set) return val_;
            else                             return val_.first;
        }


        node* left() const { return left_ ; }
        node* right() const { return right_; }

        /*the height of the tree at this node (leaves have height 1)*/
        int height() const { return height_; }
        /*the balance between the height of the left and right subtrees.
            if negative, the left subtree is deeper. if positive, the right subtree is deeper. */
        int balance() const { return balance_; }

        /*updates the left child and recalculates height and balance. should only be called
            on newly created nodes that are not required in other persistent states of the tree.*/
        void set_left(node* n)  { left_  = n; recalculate_balance(); }
        /*updates the right child and recalculates height and balance. should only be called
            on newly created nodes that are not required in other persistent states of the tree.*/
        void set_right(node* n) { right_ = n; recalculate_balance(); }

        void set_val(value_type v) { val_ = v; }

        bool has_left() const { return left_!=nullptr; }
        bool has_right() const { return right_!=nullptr; }

        bool equals_map(node* other) {
            return val_.first==other->val_.first;
        }

        bool equals_set(node* other) {
            return val_==other->val_;
        }

        bool equals(node* other) {
            if constexpr (!is_set) return equals_map(other);
            else return equals_set(other);
        }

        private:

        node* left_ = nullptr;
        node* right_ = nullptr;
        value_type val_;
        size_t height_ = 1;
        size_t balance_ = 0;

        /*recalcalates both the height and balance. automatically called when left or right is modified.*/
        void recalculate_balance() {
            size_t rh = (right()!=nullptr?right()->height():0);
            size_t lh = (left()!=nullptr?left()->height():0);
            height_=1+std::max(lh,rh);
            balance_=rh-lh;
        }

    };

    /*returns the key from a key-value-pair in a map context.
        functionally a duplicate of the private function in node*/
    T key(value_type k) {
        if constexpr (is_set) return k;
        else                  return k.first;
    }

    //balancing stuff -----------------------------------------------------------------------------------------------------------------------

    /*rotates the node and its right child. should ONLY be called during the insertion process
      for nodes where the right node should just have been created and are not in use by any
      previous states of the tree so as not to break the traversal for those other trees */
    node* left_rotate(node* n) {
        node* r = n->right();  
        node* rl = r->left();
        n->set_right(rl);
        r->set_left(n);
        return r;
    }

    /*rotates the node and its left child. should ONLY be called during the insertion process
      for nodes where the left node should just have been created and are not in use by any
      previous states of the tree so as not to break the traversal for those other trees */
    node* right_rotate(node* n) {
        node* l = n->left();  
        node* lr = l->right();
        n->set_left(lr);
        l->set_right(n);
        return l;
    }

    /*performs a balancing operation on the node, rotating either right or left if the node
      is unbalanced. does not perform copies, so must be used very carefully to not break
      traversal for different persistent states.*/
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


    //normal class stuff --------------------------------------------------------------------------------------------------------------------------

    /*root element of the tree. should be unique to every separate persistent state*/
    node* root_;

    /*array of data in case of at most ARR_LIMIT elements*/
    value_type* arr_data_;

    /*a pointer to the arena of this family*/
    arena* arena_;

    /*the amount of elements currently in the tree*/
    size_t size_;

    /*internal constructor using an existing arena with no elements*/
    setmap(arena* arena) :
        root_(nullptr), arr_data_(nullptr), arena_(arena), size_(0) {}

    /*internal constructor using an existing arena with one element*/
    setmap(value_type elem, arena* arena) :
        root_(nullptr), arr_data_(nullptr), arena_(arena), size_(1) {
            arr_data_ = new (arena_->allocate<value_type>(1)) value_type();
            *arr_data_ = elem;
        }

    /*internal constructor using an existing arena with multiple elements*/
    setmap(std::initializer_list<value_type> elems, arena* arena) :
        root_(nullptr), arr_data_(nullptr), arena_(arena), size_(0) {
            insert_list(elems.begin(),elems.end());
        }

    /*internal constructor for use in insert functions*/
    setmap(arena* arena, node* root, value_type* arr_data, size_t size) :
        root_(root), arr_data_(arr_data), arena_(arena), size_(size) {}


    //private helper functions (both modes)-------------------------------------------------------------------------------------------------------

    bool is_small() const { return arr_data_!=nullptr; }

    /*inserts a new node into the tree, also performing balancing operations.
        returns the newly created and balanced node (initial call returns new root)*/
    node* insert_helper(node* n, value_type elem) {
        if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);

        if (n->key()==key(elem)) {
            if constexpr (is_set) throw already_inserted_exception();
            else if (n->val().second==elem.second) throw already_inserted_exception();

            //map case: insert new value for existing key in the middle of the tree
            return new (arena_->allocate<node>(1)) node(n->left(), n->right(), elem);
        }

        if (!Compare{}(key(elem),n->key()))    return balance_node(new (arena_->allocate<node>(1)) node(n->left(), insert_helper(n->right(), elem), n->val()));
        else                                      return balance_node(new (arena_->allocate<node>(1)) node(insert_helper(n->left(), elem), n->right(), n->val()));
    }

    /*recursive helper for contains check*/
    bool contains_helper(node* n, T elem) const { //could be combined with find_helper to avoid code duplication
        if (n==nullptr)                 return false;
        if (n->key()==elem)             return true;
        if (!Compare{}(n->key(),elem))  return contains_helper(n->left(),  elem);
        else                            return contains_helper(n->right(), elem);
    }

    //map specific, only for internal compatibility
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    bool contains(value_type elem) const {
        if (is_small()) { //binary search through array
            value_type* start = arr_data_;
            value_type* last  = start+size_;
            value_type* mid;
            while (start!=last) {
                mid = start+(last-start)/2;
                if (*mid==elem) return true;
                if (*mid<elem)  start = mid+1;
                else            last  = mid;
            }
            throw std::runtime_error("element not found in array");
        }
        return contains_helper(root_, elem.first);
    }

    /*recursive helper for finding the node containing a specific element/key*/
    node* find_helper(node* cur, T elem) const {
        if (cur==nullptr)                return nullptr;
        if (cur->key()==elem)            return cur;
        if (Compare{}(cur->key(),elem))  return find_helper(cur->right(), elem);
        else                             return find_helper(cur->left(), elem);
    }

    /*returns either the input node or a copy of it if it is not already in the set*/
    node* change_or_copy(node* n, std::set<node*>* changed) {
        if (changed->contains(n)) return n;
        return new (arena_->allocate<node>(1)) node(n->left(), n->right(), n->val());
    }

    /*helper function for insert_list, handling the individual inserts. only allocates
      new copies of nodes that havent already been copied.*/
    node* insert_single(node* n, value_type val, std::set<node*>* changed) {
        node* newnode;
        if (n==nullptr) {
            ++size_;
            newnode = new (arena_->allocate<node>(1)) node(val);
            changed->insert(newnode);
            return newnode;
        }

        if (n->key()==key(val)) {
            --size_;
            if constexpr (is_set) throw already_inserted_exception();
            else if (n->val().second==val.second) throw already_inserted_exception();
            //map case: insert new value for existing key in the middle of the tree
            newnode = change_or_copy(n, changed);
            newnode->set_val(val);
            changed->insert(newnode);
            return newnode;
        }

        if (Compare{}(n->key(),key(val))) {
            newnode = change_or_copy(n, changed);
            newnode->set_left(n->left());
            newnode->set_right(insert_single(n->right(), val, changed));
        }
        else {
            newnode = change_or_copy(n, changed);
            newnode->set_left(insert_single(n->left(), val, changed));
            newnode->set_right(n->right());
        }

        changed->insert(newnode);
        return balance_node(newnode);
    }


    /*inserts a list one element at a time, only copying each existing node once. assumes tree mode.*/
    void insert_list(std::forward_iterator auto vbegin, std::forward_iterator auto vend) {

        assert(!is_small());// must not be used on array mode maps

        std::set<node*> changed = std::set<node*>(); //cursed hier auch std::sets zu nutzen aber naja

        for (auto vi=vbegin;vi!=vend;++vi) {
            if (is_set&&contains(*vi)) continue;
            try {
                root_=insert_single(root_, *vi, &changed);
            } catch (const already_inserted_exception& e) {}
        }
    }

    /*inserts a list of elements into the set. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap& insert_many_set(std::forward_iterator auto bi, std::forward_iterator auto ee) {
        setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,arr_data_,size_);
        
        auto ei = bi;

        while (newset->is_small()&&ei!=ee) {
            newset = &(newset->insert(*ei)); //TODO optimise for array mode, this way wastes some memory
            ++ei;
        }

        if (ei==ee) return *newset;

        newset->insert_list(ei,ee);
        return *newset;
    }

    /*inserts a list of key/value pairs into the map. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap& insert_many_map(std::forward_iterator auto bi, std::forward_iterator auto ee) {
        setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,arr_data_,size_);
        
        auto ei = bi;

        while (newmap->is_small()&&ei!=ee) {
            newmap = &(newmap->insert((*ei).first,(*ei).second)); //TODO optimise for array mode, this way wastes some memory
            ++ei;
        }

        if (ei==ee) return *newmap;

        newmap->insert_list(ei,ee);
        return *newmap;
    }

public:

    //public constructors-------------------------------------------------------------------------------------------------------------------------------------

    /*regular constructor creating the first member of a new "family" of setmaps
      with no starting elements*/
    setmap() :
        setmap(new arena) {}

    /*regular constructor creating the first member of a new "family" of setmaps
      with one starting element*/
    setmap(value_type elem) :
        setmap(elem, new arena) {}

    /*regular constructor creating the first member of a new "family" of setmaps
      with multiple starting elements*/
    setmap(std::initializer_list<value_type> elems) :
        setmap(elems, new arena) {}

    /*copy constructor*/
    setmap(const setmap& other) :
        root_(other.root_), arr_data_(other.arr_data_), 
        arena_(other.arena_), size_(other.size_) {}

    /*copy assignment operator*/
    setmap& operator=(const setmap& other) {
        root_     = other.root_;
        arr_data_ = other.arr_data_;
        arena_    = other.arena_;
        size_     = other.size_;
        return *this;
    }

    /*standard destructor*/
    ~setmap() {
        //if (root_) root_->~node();
    }

    //public functions (both modes)------------------------------------------------------------------------------------------------------------------------------

    /*returns whether the root element is null or not*/
    bool empty() { return root_==nullptr&&arr_data_==nullptr; }

    /*iterator going forward through the tree, using a stack implementation*/
    struct iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = setmap::value_type;
        using iterator_category = std::forward_iterator_tag;

        const setmap* setmap_;
        bool is_arr_;
        node* current_; //current_==nullptr means end()
        size_t idx_;    //idx_==size_ means end()
        std::vector<node*> stack_ = std::vector<node*>();

        iterator() = default;

        iterator(const setmap* set) :
            setmap_(set), is_arr_(set->is_small()), current_(set->root_), idx_(0) {
                if (is_arr_) return;
                if (current_==nullptr) return;
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

        iterator(const setmap* set, node* n) :
            setmap_(set), is_arr_(false), current_(set->root_) {
                if (n==nullptr) {current_=nullptr; return;} //end() case
                T nval = n->key();
                if (!set->contains(nval)) { current_=nullptr; return; }
                current_ = set->root_;
                if (current_==n) return;
                while (current_!=n) {
                    if (current_==nullptr) throw std::runtime_error("bad node");
                    stack_.push_back(current_);
                    if (current_->key()<nval) current_=current_->right();
                    else if (current_->key()>nval) current_=current_->left();
                }
            }

        iterator(const setmap* set, T elem, void*) : //just for find() and operator[]
            setmap_(set), is_arr_(set->is_small()), current_(set->root_) {
                if (is_arr_) { //binary search through array to find element
                    value_type* base = setmap_->arr_data_;
                    value_type* start = base;
                    value_type* last = start+setmap_->size_;
                    value_type* mid;
                    while (start!=last) {
                        mid = start + (last - start) / 2;
                        if constexpr (is_set) {
                            if (*mid==elem)                 { idx_ = mid - base; return; }
                            if (Compare{}(*mid,elem))       start = mid+1;
                            else                            last  = mid;
                        } 
                        if constexpr (!is_set) {
                            if (mid->first==elem)           { idx_ = mid - base; return; }
                            if (Compare{}(mid->first,elem)) start = mid+1;
                            else                            last  = mid;
                        } 
                    }
                    idx_ = setmap_->size_; return; //not found, return end()
                }
                if (!set->contains(elem)) { current_=nullptr; return; }
                if (current_->key()==elem) return;
                while (current_->key()!=elem) {
                    if (current_->key()<elem) current_=current_->right();
                    else if (current_->key()>elem) {
                        stack_.push_back(current_);
                        current_=current_->left();
                    }
                    if (current_==nullptr) throw std::runtime_error("bad node");
                }
            }

        iterator(const setmap* sm, size_t idx) :
            setmap_(sm), is_arr_(true), idx_(idx) {}

        /*returns the element of the node currently pointed to by the iterator*/
        const value_type& operator*() const { if (is_arr_) return setmap_->arr_data_[idx_]; return current_->val(); }
        value_type* operator->() const { if (is_arr_) return &setmap_->arr_data_[idx_]; return &current_->val(); } //not const for weird compatibility

        /*increments the iterator by one*/
        iterator& operator++() {
            if (is_arr_) { ++idx_; return *this; }
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

        bool operator==(const iterator& other) const { if (is_arr_) return setmap_==other.setmap_&&idx_==other.idx_;  return setmap_==other.setmap_ && ((current_==nullptr&&other.current_==nullptr) || (current_!=nullptr&&other.current_!=nullptr && current_->equals(other.current_))); }
        bool operator!=(const iterator& other) const { if (is_arr_) return setmap_==other.setmap_&&idx_!=other.idx_;  return !(*this==other); }

        bool operator<(const iterator& other) const { if (is_arr_) return idx_<other.idx_;  return this->current_->key()<other->current_->key(); };
        bool operator>(const iterator& other) const { if (is_arr_) return idx_>other.idx_;  return this->current_->key()>other->current_->key(); };
        bool operator<=(const iterator& other) const { if (is_arr_) return idx_<=other.idx_;  return this->current_->key()<=other->current_->key(); };
        bool operator>=(const iterator& other) const { if (is_arr_) return idx_>=other.idx_;  return this->current_->key()>=other->current_->key(); };
    };
    /*returns an iterator to the first and smallest element in the setmap*/
    iterator begin() const { return iterator(this); }
    /*returns an iterator that acts as a sentinel after the last element of the setmap*/
    iterator end() const { if (is_small()) return iterator(this, size_); else return iterator(this, nullptr); }

    /*the amount of elements currently in the tree*/
    size_t size() const { return size_; }

    /*checks whether a key is included in the set
        (no point in checking for a key/value pair in a map, just check for the key)*/
    bool contains(T elem) const {         
        if (is_small()) { //binary search through array
            value_type* base = arr_data_;
            value_type* start = base;
            value_type* last = start+size_;
            value_type* mid;
            while (start!=last) {
                mid = start + (last - start) / 2;
                if constexpr (is_set) {
                    if (*mid==elem)                 { return true; }
                    if (Compare{}(*mid,elem))       start = mid+1;
                    else                            last  = mid;
                } 
                if constexpr (!is_set) {
                    if (mid->first==elem)           { return true; }
                    if (Compare{}(mid->first,elem)) start = mid+1;
                    else                            last  = mid;
                } 
            }
            return false;
        }
        return contains_helper(root_, elem); 
    }
    bool count(T elem) const { return contains(elem); }

    /*checks whether a list of keys in included in the set
        (no point in checking for a key/value pair in a map, just check for the key)*/
    bool contains_all(std::initializer_list<T> elems) const {
        for (auto v : elems) {
            if (!contains(v)) return false;
        }
        return true;
    }

    /*returns an iterator to the node containing elem.
        returns end() if elem isn't in the set.*/
    iterator find(T elem) const { return iterator(this, elem, nullptr); }

    /*compares whether 2 setmaps contain all of the same elements*/
    bool operator==(setmap other) const {
        //TODO just use this==other?
        if (size()!=other.size()) return false;
        for (auto elem : other) if (!contains(elem)) return false;
        return true;
    }

    /*compares whether 2 setmaps do not contain all of the same elements*/
    bool operator!=(setmap other) const { return !(*this==other); }

    setmap& insert(std::forward_iterator auto bi, std::forward_iterator auto ee) {
        if constexpr (is_set) return insert_many_set(bi, ee);
        else return insert_many_map(bi,ee);
    }


    //set specific functions---------------------------------------------------------------------------------------------------------------------------------------------

    /*inserts an element into the set. returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap& insert(T elem) {
        if (size()<ARR_LIMIT) {
            size_t newsize = size_+1;
            T* new_arr = new (arena_->allocate<T>(newsize)) T();
            bool ins = false;
            for (auto si = arr_data_, di = new_arr, se = arr_data_+size_; si != se || !ins; ++di) {
                if (si != se && elem == *si) { // already here
                    arena_->deallocate(sizeof(T)*(newsize));
                    return *this;
                }
                if (!ins && (si == se || elem < (*si)))
                    *di = elem, ins = true, ++si;
                else
                    *di = *si++;
            }
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,nullptr,new_arr,newsize);
            return *newset;
        }

        if (root_==nullptr) {
            node* newnode = new (arena_->allocate<node>(1)) node(elem);
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, nullptr, 1);
            return *newset;
        }
        node* newnode;
        try { 
            newnode = insert_helper(root_, elem);
        } catch (const already_inserted_exception&) {
            return *this;
        }
        setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode, nullptr, size_+1);
        return *newset;
    }



    /*inserts a list of elements into the set. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap& insert(std::initializer_list<T> elems) {
        return insert_many_set(elems.begin(), elems.end());
    }

    //map specific functions----------------------------------------------------------------------------------------------------------------------

    /*returns the value for a given key. throws exception if the key does not exist in this map*/
    template<typename U = V,  typename = std::enable_if_t<!std::is_void_v<U>>>
    const V operator[](T key) const {
        if (is_small()) {
            auto it = iterator(this, key, nullptr);
            if (it==end()) throw std::runtime_error("key not found in array");
            return it->second;
        }
        auto n = find_helper(root_, key);
        if (n==nullptr) throw std::runtime_error("key not found in tree");
        return n->val().second;
    }

    /*inserts a key/value pair into the map. returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap insert(T key, U value) {

        auto kv = std::pair(key,value);
        size_t newsize = size_+1;//!contains(key); //cursed 

        //staying in array mode
        if (newsize<=ARR_LIMIT&&is_small()) {
            value_type* new_arr = new (arena_->allocate<value_type>(newsize)) value_type[newsize];
            bool ins = false;
            for (auto si = arr_data_, di = new_arr, se = arr_data_+size_; si != se || !ins; ++di) {
                if (si != se) {
                    if (key == (*si).first) { // already here
                        if ((*si).second==value) {
                            arena_->deallocate(sizeof(value_type)*(newsize));
                            return *this;
                        }
                        else {
                            *di = kv, ins = true, ++si;
                            arena_->deallocate(sizeof(value_type)); //dealloc last elem
                            --newsize;
                            continue;
                        }
                    }
                }
                if (!ins && (si == se || key < (*si).first))
                    *di = kv, ins = true;
                else
                    *di = *si++;
            }
            return setmap(arena_,nullptr,new_arr,newsize);
            // setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,nullptr,new_arr,newsize);
            // return *newmap;
        }
        

        //switch from arr to tree
        if (is_small()&&newsize>ARR_LIMIT) {
            node* newroot = nullptr;
            for (auto si = arr_data_, se = arr_data_+size_; si!=se; ++si) {
                newroot = insert_helper(newroot, *si);
            }
            //newroot = insert_helper(newroot, kv);
            try {
                newroot = insert_helper(newroot, kv);
            } catch (const already_inserted_exception&) { 
                --newsize; //if new element not actually new then switch to tree anyway, would waste memory otherwise
            } 
            return setmap(arena_, newroot, nullptr, newsize);
            // setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_, newroot, nullptr, newsize);
            // return *newmap;
        }

        //currently dead code
        if (root_==nullptr) {
            node* newnode = new (arena_->allocate<node>(1)) node(kv);
            return setmap(arena_, newnode, nullptr, 1);
            // setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, nullptr, 1);
            // return *newmap;
        }

        //normal tree mode insert
        node* newnode;
        try {
            newnode = insert_helper(root_, kv);
        } catch (const already_inserted_exception&) {
            return *this;
        }
        return setmap(arena_,newnode,nullptr,newsize);
        // setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,nullptr,newsize);
        // return *newmap;
    }

    /*inserts a list of key/value pairs into the map. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap& insert(std::initializer_list<std::pair<T,V>> elems) {
        return insert_many_map(elems.begin(), elems.end());
    }

    using str = std::string;

    /*creates an xdot representation of the set/map and saves it in a specified file*/
    str save_dot() const {
        if (is_small()) return "small mode"; 
        str where="/tmp/SMdot.tmp";
        std::ofstream out(where);
        out << "digraph setmap {\n";
        for (auto it = begin();it!=end();++it){
            auto n = it.current_;
            if (n->has_left()) {
                if constexpr (is_set) out << *it;
                else out << "\""<<it->first<<": "<<it->second<<"\"";
                out << " -> ";
                auto l = n->left()->val();
                if constexpr (is_set) out << l;
                else out << "\""<<l.first<<": "<<l.second<<"\"";
                out << ";\n";
            } 
            if (n->has_right()) {
                if constexpr (is_set) out << *it;
                else out << "\""<<it->first<<": "<<it->second<<"\"";
                out << " -> ";
                auto r = n->right()->val();
                if constexpr (is_set) out << r;
                else out << "\""<<r.first<<": "<<r.second<<"\"";
                out << ";\n";
            }
        }
        out<<"}";
        out.close();
        return where;
    }

 
static_assert(std::forward_iterator<typename setmap::iterator>);
};



template<typename T, typename V = void, typename Compare = std::less<T>>
struct freezer {
    using sm = setmap<T,V,Compare>;
    sm sm_;
    sm* ptr_;

    freezer(sm* s) :
        sm_(*s), ptr_(s) {}

    ~freezer() {
        *ptr_ = sm_;
    }
};

}
