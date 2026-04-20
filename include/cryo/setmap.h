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

// template <typename P>
// struct is_unique_ptr : std::false_type {};

// template <typename P, typename Deleter>
// struct is_unique_ptr<std::unique_ptr<P, Deleter>> : std::true_type {};

// template <typename P>
// inline constexpr bool is_unique_ptr_v = is_unique_ptr<P>::value;

// template <typename T>
// auto move_if_unique_ptr(T& val) {
//     if constexpr (is_unique_ptr_v<T>) {
//         return val;
//     } else {
//         return val;
//     }
// }

// #define MOVE_OR_NOT(X) move_if_unique_ptr(X)

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
    bool is_set = std::is_void_v<V>;

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
            if (has_left())  left_->~node();
            if (has_right()) right_->~node();
        }

        value_type& val() { return val_; }

        T& key() {
            if constexpr (std::is_void_v<V>) return val_;
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
            if constexpr (!std::is_void_v<V>) return equals_map(other);
            else return equals_set(other);
        }

        private:

        node* left_ = nullptr;
        node* right_ = nullptr;
        value_type val_;
        int height_ = 1;
        int balance_ = 0;

        /*recalcalates both the height and balance. automatically called when left or right is modified.*/
        void recalculate_balance() {
            int rh = (right()!=nullptr?right()->height():0);
            int lh = (left()!=nullptr?left()->height():0);
            height_=1+std::max(lh,rh);
            balance_=rh-lh;
        }

    };

    /*returns the key from a key-value-pair in a map context.
        functionally a duplicate of the private function in node*/
    T key(value_type k) {
        if constexpr (std::is_void_v<V>) return k;
        else                             return k.first;
    }

    //balancing stuff -------------------------------------------

    /*rotates the node and its right child. should ONLY be called during the insertion process
        for nodes where the right node should just have been created and are not in use by any
        previous states of the tree so as not to break the traversal for those other trees */
    node* left_rotate(node* n) {
        node* r = n->right();  //
        node* rl = r->left();
        n->set_right(rl);
        r->set_left(n);
        return r;
    }

    /*rotates the node and its left child. should ONLY be called during the insertion process
        for nodes where the left node should just have been created and are not in use by any
        previous states of the tree so as not to break the traversal for those other trees */
    node* right_rotate(node* n) {
        node* l = n->left();  //
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


    //normal class stuff ----------------------------------------------

    /*root element of the tree. should be unique to every separate persistent state*/
    node* root_;

    /*a pointer to the arena of this family*/
    arena* arena_;

    /*the amount of elements currently in the tree*/
    size_t size_;

    /*internal constructor using an existing arena with no elements*/
    setmap(arena* arena) :
        root_(nullptr), arena_(arena), size_(0) {}

    /*internal constructor using an existing arena with one element*/
    setmap(value_type elem, arena* arena) :
        root_(nullptr), arena_(arena), size_(1) {
            root_=new (arena_->allocate<node>(1)) node(elem);
        }

    /*internal constructor using an existing arena with multiple elements*/
    setmap(std::initializer_list<value_type> elems, arena* arena) :
        root_(nullptr), arena_(arena), size_(0) {
            insert_list(elems);
        }

    /*internal constructor for use in insert functions*/
    setmap(arena* arena, node* root, size_t size) :
        root_(root), arena_(arena), size_(size) {}


    //private helper functions (both modes)---------------------------

    /*inserts a new node into the tree, also performing balancing operations.
        returns the newly created and balanced node (initial call returns new root)*/
    node* insert_helper(node* n, value_type elem) {
        if (n==nullptr) return new (arena_->allocate<node>(1)) node(elem);

        if (n->key()==key(elem)) {
            if (is_set) throw std::runtime_error("set already contains element");
            //map case: insert new value for existing key in the middle of the tree
            return new (arena_->allocate<node>(1)) node(n->left(), n->right(), elem);
        }

        if (!Compare{}(key(elem),n->key()))    return balance_node(new (arena_->allocate<node>(1)) node(n->left(), insert_helper(n->right(), elem), n->val()));
        else                  return balance_node(new (arena_->allocate<node>(1)) node(insert_helper(n->left(), elem), n->right(), n->val()));
    }

    /*recursive helper for contains check*/
    bool contains_helper(node* n, T elem) const {
        if (n==nullptr)                 return false;
        if (n->key()==elem)             return true;
        if (!Compare{}(n->key(),elem))  return contains_helper(n->left(), elem);
        else                            return contains_helper(n->right(), elem);
    }

    //map specific, only for internal compatibility
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    bool contains(value_type elem) const {
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
            size_++;
            newnode = new (arena_->allocate<node>(1)) node(val);
            changed->insert(newnode);
            return newnode;
        }

        if (n->key()==key(val)) {
            size_--;
            if (is_set) throw std::runtime_error("set already contains element");
            //map case: insert new value for existing key in the middle of the tree
            newnode = change_or_copy(n, changed);
            newnode->set_val(val);
            changed->insert(newnode);
            return newnode;
        }

        if (val>n->val()) {
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

    /*inserts a list one element at a time, only copying each existing node once*/
    void insert_list(std::initializer_list<value_type> vals) {
        std::set<node*> changed = std::set<node*>(); //cursed hier auch std::sets zu nutzen aber naja

        for (auto v : vals) {
            if (is_set&&contains(v)) continue;
            root_=insert_single(root_, v, &changed);
        }
    }

public:

    //public constructors---------------------------------------------------------

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

    /*standard destructor*/
    ~setmap() {
        if (root_) root_->~node();
    }

    //public functions (both modes)--------------------------------------------------

    /*returns whether the root element is null or not*/
    bool empty() { return root_==nullptr; }

    //setmap& operator=(setmap& other) = default;

    /*iterator going forward through the tree, using a stack implementation*/
    struct iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = setmap::value_type;
        using iterator_category = std::forward_iterator_tag;

        const setmap* setmap_;
        node* current_;
        std::vector<node*> stack_ = std::vector<node*>();

        iterator() = default;

        iterator(const setmap* set) :
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

        iterator(const setmap* set, node* n) :
            setmap_(set), current_(set->root_) {
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

        iterator(const setmap* set, T elem, int*) : //just for find()
            setmap_(set), current_(set->root_) {
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

        /*returns the element of the node currently pointed to by the iterator*/
        const value_type& operator*() const { return current_->val(); }
        value_type* operator->() const { return &current_->val(); } //not const for weird compatibility

        /*increments the iterator by one*/
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

        bool operator==(const iterator& other) const { return setmap_==other.setmap_ && ((current_==nullptr&&other.current_==nullptr) || (current_!=nullptr&&other.current_!=nullptr && current_->equals(other.current_))); }
        bool operator!=(const iterator& other) const { return !(*this==other); }

        bool operator<(const iterator& other) const { return this->current_->key()<other->current_->key(); };
        bool operator>(const iterator& other) const { return this->current_->key()>other->current_->key(); };
        bool operator<=(const iterator& other) const { return this->current_->key()<=other->current_->key(); };
        bool operator>=(const iterator& other) const { return this->current_->key()>=other->current_->key(); };
    };
    /*returns an iterator to the first and smallest element in the tree*/
    iterator begin() const { return iterator(this); }
    /*returns an iterator that acts as a sentinel after the last element of the tree*/
    iterator end() const { return iterator(this, nullptr); }

    /*the amount of elements currently in the tree*/
    size_t size() const { return size_; }

    /*checks whether a key is included in the set
        (no point in checking for a key/value pair in a map, just check for the key)*/
    bool contains(T elem) const { return contains_helper(root_, elem); }
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
        if (size()!=other.size()) return false;
        // return contains_all(other);
        for (auto elem : other) if (!contains(elem)) return false;
        return true;
    }

    /*compares whether 2 setmaps do not contain all of the same elements*/
    bool operator!=(setmap other) const { return !(*this==other); }


    //set specific functions------------------------------------------

    /*inserts an element into the set. returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap insert(T elem) {
        if (contains(elem)) return this;
        if (root_==nullptr) {
            node* newnode = new (arena_->allocate<node>(1)) node(elem);
            setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, 1);
            return *newset;
        }
        node* newnode = insert_helper(root_, elem);
        setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,size_+1);
        return *newset;
    }
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap emplace(T elem) {
        return insert(elem);
    }

    /*inserts a list of elements into the set. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap insert(std::initializer_list<T> elems) {
        if (contains_all(elems)) return this;
        setmap* newset = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,size_);
        newset->insert_list(elems);
        return *newset;
    }
    template <typename U = V, typename = std::enable_if_t<std::is_void_v<U>>>
    setmap emplace(std::initializer_list<T> elems) {
        return insert(elems);
    }

    //map specific functions------------------------------------------

    /*returns the value for a given key. throws exception if the key does not exist in this map*/
    template<typename U = V,  typename = std::enable_if_t<!std::is_void_v<U>>>
    const V operator[](T key) const {
        auto kek = find_helper(root_, key);
        if (kek==nullptr) throw std::runtime_error("key does not exist here");
        return kek->val().second;
    }

    /*inserts a key/value pair into the map. returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap insert(T key, U value) {
        if (root_==nullptr) {
            node* newnode = new (arena_->allocate<node>(1)) node(std::pair(key,value));
            setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_, newnode, 1);
            return *newmap;
        }
        size_t newsize = size_+!contains(key); //cursed
        node* newnode = insert_helper(root_, std::pair(key,value));
        setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,newnode,newsize);
        return *newmap;
    }
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap emplace(T key, U value) {
        return insert(key, value);
    }

    /*inserts a list of key/value pairs into the map. only copies what is necessary and returns a new persistent copy with a new root element.*/
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap insert(std::initializer_list<std::pair<T,V>> elems) {
        setmap* newmap = new (arena_->allocate<setmap>(1)) setmap(arena_,root_,size_);
        newmap->insert_list(elems);
        return *newmap;
    }
    template <typename U = V, typename = std::enable_if_t<!std::is_void_v<U>>>
    setmap emplace(std::initializer_list<std::pair<T,V>> elems) {
        return insert(elems);
    }

    //some debug stuff, could be private, could be deleted later ------------------------

    int depthchecker(node* n) const {
        if (n==nullptr) return 0;
        return 1+(std::max(depthchecker(n->left()),depthchecker(n->right())));
    }

    /*returns the maximum depth of the tree for debug purposes*/
    int checkmaxdepth() const {
        return depthchecker(root_);
    }

    void printer(node* n) const {
        if (n==nullptr) return;
        if constexpr ((std::is_same_v<T, int> || std::is_same_v<T, std::string>)&&(std::is_same_v<V, int> || std::is_same_v<V, std::string>)) std::cout<<"val: "<<n->val()<<", height: "<<n->height()<<std::endl<<"-> "<<
        (n->has_left()?std::to_string(n->left()->val()):"X")<<" "<<
        (n->has_right()?std::to_string(n->right()->val()):"X")<<std::endl<<std::endl;
        printer(n->left());
        printer(n->right());
    }

    /*prints the tree for debug purposes*/
    void printtree() const {
        if constexpr (!std::is_same_v<T, int> && !std::is_same_v<T, std::string>) return;
        if constexpr (!std::is_same_v<V, int> && !std::is_same_v<V, std::string>) return;
        printer(root_);
    }

 
static_assert(std::forward_iterator<typename setmap::iterator>);
};

}
