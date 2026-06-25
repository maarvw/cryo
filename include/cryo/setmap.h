#pragma once

#include <fe/arena.h>
#include <cstddef>
#include <cstring>
#include <algorithm>
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
            else                  return val_.first;
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

    /*builds balanced tree from array  of elements, for switching from array to tree mode*/    
    node* build_balanced(value_type* sorted, size_t lo, size_t hi) {
        if (lo >= hi) return nullptr;
        size_t mid = lo + (hi - lo) / 2;
        auto* n = new (arena_->allocate<node>(1)) node(sorted[mid]);
        n->set_left(build_balanced(sorted, lo, mid));
        n->set_right(build_balanced(sorted, mid+1, hi));
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
    node* insert_helper(node* n, value_type elem, bool& inc) {
        if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);

        if (n->key()==key(elem)) { //key already exists
            inc = false; //no increasing size
            if constexpr (is_set) return n; 
            else if (n->val().second==elem.second) return n; 

            //map case: insert new value for existing key in the middle of the tree
            return new (arena_->allocate<node>(1)) node(n->left(), n->right(), elem);
        }

        node *l, *r;
        if (!Compare{}(key(elem),n->key())) {
            l = n->left();
            r = insert_helper(n->right(), elem, inc);
        }
        else {
            l = insert_helper(n->left(), elem, inc);
            r = n->right();
        }

        if (l==n->left()&&r==n->right()) return n; //if both children are the same dont create new node
        else return balance_node(new (arena_->allocate<node>(1)) node(l, r, n->val()));
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
      new copies of nodes that havent already been copied and mutates already copied ones in-place.*/
    node* insert_single_tree(node* n, value_type val, std::set<node*>* changed) {
        node* newnode;
        if (n==nullptr) {
            ++size_;
            newnode = new (arena_->allocate<node>(1)) node(val);
            changed->insert(newnode);
            return newnode;
        }

        if (n->key()==key(val)) {
            --size_;
            if constexpr (is_set) return n; 
            else if (n->val().second==val.second) return n; 
            //map case: insert new value for existing key in the middle of the tree
            newnode = change_or_copy(n, changed);
            newnode->set_val(val);
            changed->insert(newnode);
            return newnode;
        }

        newnode = change_or_copy(n, changed);
        node *l, *r;

        if (Compare{}(n->key(),key(val))) {
            l = n->left(); 
            r = insert_single_tree(n->right(), val, changed); 
        }
        else {
            l = insert_single_tree(n->left(), val, changed);
            r = n->right();
        }

        newnode->set_left(l);
        newnode->set_right(r);

        changed->insert(newnode);
        return balance_node(newnode);
    }

    /*inserts a list one element at a time, only copying each existing node once. assumes tree mode.*/
    void insert_list(std::forward_iterator auto vbegin, std::forward_iterator auto vend) {

        assert(!is_small());// must not be used on array mode maps
        

        std::set<node*> changed = std::set<node*>(); //cursed hier auch std::sets zu nutzen aber naja

        for (auto vi=vbegin;vi!=vend;++vi) {
            if (is_set&&contains(*vi)) continue;
            root_=insert_single_tree(root_, *vi, &changed);
        }
    }

    /*creates a (sorted) std::vector from the elements in the setmap*/
    std::vector<value_type> to_vec() {
        std::vector<value_type> v;
        auto it = begin(), e=end();
        while (it!=e) { v.push_back(*it); ++it; }
        return v;
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
                //if (!set->contains(elem)) { current_=nullptr; return; }
                if (current_==nullptr) return;
                if (current_->key()==elem) return;
                while (current_->key()!=elem) {
                    if (current_->key()<elem) current_=current_->right();
                    else if (current_->key()>elem) {
                        stack_.push_back(current_);
                        current_=current_->left();
                    }
                    if (current_==nullptr) return;
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

    /*inserts a batch of elements between the 2 iterators*/
    setmap insert(std::forward_iterator auto eb, std::forward_iterator auto ee, bool sort_elems=true ) {
        if (!is_small()) {
            setmap newmap =  setmap(arena_,root_,nullptr,size_);
            newmap.insert_list(eb, ee);
            return newmap;
        }
        
        auto ei = eb;

        if (sort_elems) std::sort(eb,ee);


        value_type* newarr = new (arena_->allocate<value_type>(ARR_LIMIT)) value_type[ARR_LIMIT];
        size_t newsize = size_;

        //combine old arr and new elements into new arr
        for (auto ai = arr_data_, ae = arr_data_+size_, ni = newarr; newsize<ARR_LIMIT;) {
            if (ei==ee) { //end of new elements, just insert the rest of old array
                while (newsize<ARR_LIMIT && ai!=ae) {
                    *ni=*ai;
                    ++ni, ++ai;
                }
                break;
            }
            if (ai==ae) { //end of old array, just insert rest of new elements
                while (newsize<ARR_LIMIT && ei!=ee) {
                    *ni=*ei;
                    ++ni, ++ei;
                }
                break;
            }
            if (ai->first == ei->first) { ++ei; continue; }
            if (ai->first <  ei->first) { *ni = *ai; ++ai; ++ni; ++newsize; }
            else                        { *ni = *ei; ++ei; ++ni; ++newsize; }
        }

        
        //check if limit reached, then switch to tree
        if (newsize==ARR_LIMIT) {
            node* newroot = build_balanced(newarr, 0, newsize-1);
            setmap newmap = setmap(arena_,newroot,nullptr,newsize);
            newmap.insert_list(ei, ee);
            return newmap;
        }
        
        //deallocate unused array space
        arena_->deallocate(sizeof(value_type)*(ARR_LIMIT-newsize));

        return setmap(arena_, nullptr, newarr, newsize);
    }

    /*returns a new setmap containing the elements of both setmaps. in map mode, keys in 'this' are overwritten by 'other'*/
    setmap merge(setmap other) {
        auto v = other.to_vec();
        return insert(v.begin(), v.end(), false);
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
        newnode = insert_helper(root_, elem,nullptr);
        if (newnode==root_) return *this; //if elem already existed
        setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode, nullptr, size_+1);
        return *newset;
    }



    /*inserts a list of elements into the set. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap& insert(std::initializer_list<T> elems) {
        return insert(elems.begin(), elems.end());
    }

    //map specific functions----------------------------------------------------------------------------------------------------------------------

    /*returns the value for a given key. throws exception if the key does not exist in this map*/
    template<typename U = V,  typename = std::enable_if_t<!std::is_void_v<U>>>
    const V& operator[](T key) const {
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
        if (is_small()) {
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

            //check if limit reached, then switch to tree
            if (newsize==ARR_LIMIT) {
                node* newroot = build_balanced(new_arr, 0, newsize-1);
                return setmap(arena_,newroot,nullptr,newsize);
            }


            return setmap(arena_,nullptr,new_arr,newsize);
        }
        

        //currently dead code
        if (root_==nullptr) {
            node* newnode = new (arena_->allocate<node>(1)) node(kv);
            return setmap(arena_, newnode, nullptr, 1);
        }

        //normal tree mode insert
        node* newnode;
        bool inc=true; //increase size or not
        newnode = insert_helper(root_, kv, inc);
        if (newnode==root_) return *this; //if nothing changed
        if (!inc) --newsize; //if key already existed but value was changed
        return setmap(arena_,newnode,nullptr,newsize);
    }

    /*inserts a list of key/value pairs into the map. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap insert(std::initializer_list<std::pair<T,V>> elems) {
        return insert(elems.begin(), elems.end());
    }

    /*for remapping a list of keys already in the map*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap mutate_keys(std::forward_iterator auto ib, std::forward_iterator auto ie) {
        if (!is_small()) return insert(ib,ie);
        std::sort(ib,ie);
        value_type* newarr = new (arena_->allocate<value_type>(size_)) value_type[size_];
        memcpy(newarr, arr_data_, sizeof(value_type)*size_);
        auto ii = ib;
        for (auto si = arr_data_, se = arr_data_+size_; si!=se; ++si) {
            if (si->first==ii->first) *si=*ii;
        }
        return setmap(arena_,nullptr,newarr,size_);
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
