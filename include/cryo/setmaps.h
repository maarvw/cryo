#pragma once

#include </home/marvin/Uni/Bachelorarbeit/MimIR/submodules/fe/include/fe/arena.h>
#include <deque>
#include <type_traits>


using arena = fe::Arena;


namespace cryo {
    
template<typename K, typename V = void, size_t ARR_LIM = 16, typename Compare = std::less<K>>
class setmaps {
    using arena = fe::Arena;
    
    using value_type = std::conditional_t<std::is_void_v<V>, K, std::pair<K, V>>;
    using mapped_type = V;

    arena arena_;

    static constexpr bool is_set() {return  std::is_void_v<V>; }
    static constexpr bool is_map() {return !std::is_void_v<V>; }

    static K key(value_type val) {
        if constexpr (is_set()) { return val;       }
        else                    { return val.first; }
    }

    class node {
        node* left_;
        node* right_;
        value_type val_;
        int height_ = 1;
        int balance_ = 0;    

        constexpr node(value_type val)  noexcept
            : val_(val) {}

        constexpr node(node* l, node* r, value_type val) noexcept
            : left_(l)
            , right_(r)
            , val_(val) {
                recalculate_balance();
        }

        node* left()    const  { return left_;  }
        node* right()    const { return right_; }
        value_type val() const { return val_;   }

        void set_left (node* n) { left_  = n; recalculate_balance(); }
        void set_right(node* n) { right_ = n; recalculate_balance(); }

        bool has_left () { return left_  != nullptr; }
        bool has_right() { return right_ != nullptr; }

        void recalculate_balance() {
            int rh = (right_ != nullptr ? right_->height_ : 0);
            int lh = (left_  != nullptr ? left_ ->height_ : 0);
            height_  = 1+std::max(lh,rh);
            balance_ = rh-lh;
        }

        static node* left_rotate(node* n) {
            node* r =  n->right_;  
            node* rl = r->left_;
            n->set_right(rl);
            r->set_left(n);
            return r;
        }
        static node* right_rotate(node* n) {
            node* l  = n->left_;  
            node* lr = l->right_;
            n->set_left(lr);
            l->set_right(n);
            return l;
        }
        static node* balance_node(node* n) {
            int bal = n->balance_;
            if (bal<-1) {
                return right_rotate(n);
            }
            else if (bal>1) {
                return left_rotate(n);
            }
            return n;
        }

        bool contains(value_type val) {
            if (key(val) == key(val_)) return true;
            if (key(val) <  key(val_)) {
                if (has_left()) return false;
                else            return left_->contains(val);
            }
            if (key(val) >  key(val_)) {
                if (has_right()) return false;
                else             return right_->contains(val);
            }
        }
    };

    node* build_balanced(value_type* sorted, size_t lo=0, size_t hi=ARR_LIM-1) {
        if (lo >= hi) return nullptr;
        size_t mid = lo + (hi - lo) / 2;
        auto [n, _] = make_node(sorted[mid]);
        n->set_left(build_balanced(sorted, lo, mid));
        n->set_right(build_balanced(sorted, mid+1, hi));
        return n;
    }

    class arr {
        size_t size_ = 0;
        value_type arr_[];

        constexpr arr() noexcept = default;
        constexpr arr(size_t size) noexcept 
            : size_(size) {}

        constexpr value_type* begin() noexcept { return arr_; }
        constexpr value_type* end() noexcept { return arr_ + size_; }
        constexpr value_type const* begin() const noexcept { return arr_; }
        constexpr value_type const* end() const noexcept { return arr_ + size_; }
    };

    std::pair<arr*, arena::State> allocate(size_t size) {
        auto bytes = sizeof(arr) + size * sizeof(value_type);
        auto state = arena_.state();
        auto buff  = arena_.allocate(bytes, alignof(arr));
        auto data  = new (buff) arr(size);
        return {data, state};
    }

    std::pair<node*, arena::State> make_node(node* l, node* r, value_type val) {
        auto bytes = sizeof(node) + sizeof(value_type);
        auto state = arena_.state();
        auto buff  = arena_.allocate(bytes, alignof(node));
        auto data = new (buff) node(l, r, val);
        return {data, state};
    }
    std::pair<node*, arena::State> make_node(value_type val) {
        return make_node(nullptr, nullptr, val);
    }


    class setmap {
        
        uintptr_t data_ = 0;

        enum class Tag : uintptr_t { Null, Uniq, Array, Node };
        constexpr Tag tag() const noexcept { return Tag(data_ & uintptr_t(0b11)); }

        template<class T>
        constexpr T* ptr() const noexcept {
            return std::bit_cast<T*>(data_ & (uintptr_t(-2) << uintptr_t(2)));
        }

        constexpr value_type* isa_uniq() const noexcept { return tag() == Tag::Uniq ? ptr<value_type>() : nullptr; }
        constexpr arr*        isa_data() const noexcept { return tag() == Tag::Data ? ptr<arr       >() : nullptr; }
        constexpr node*       isa_node() const noexcept { return tag() == Tag::Node ? ptr<node      >() : nullptr; }

        constexpr setmap(const setmap&) noexcept = default;
        constexpr setmap(setmap&&) noexcept      = default;
        constexpr setmap() noexcept              = default; ///< Null setmap
        constexpr setmap(value_type d) noexcept
            : data_(uintptr_t(&d) | uintptr_t(Tag::Uniq)) {} ///< Uniq setmap.
        constexpr setmap(const arr* data) noexcept
            : data_(uintptr_t(data) | uintptr_t(Tag::Data)) {} ///< Array setmap.
        constexpr setmap(node* node) noexcept
            : data_(uintptr_t(node) | uintptr_t(Tag::Node)) {} ///< Node setmap.

        constexpr setmap& operator=(const setmap&) noexcept = default;

        constexpr size_t size() const noexcept {
            if (isa_uniq()) return 1;
            if (auto d = isa_data()) return d->size;
            if (auto n = isa_node()) return n->size;
            return 0; // empty
        }

        constexpr bool empty() const noexcept {
            assert(tag() != Tag::Node || !ptr<node>()->is_root());
            return data_ == 0;
        }

        bool contains(value_type d) const noexcept {
            if (auto u = isa_uniq()) return d == *u;

            if (auto data = isa_data()) {
                for (auto e : *data)
                    if (d == *e) return true;
                return false;
            }

            if (auto n = isa_node()) return n->contains(d);

            return false;
        }

    
        class iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = value_type*;
            using pointer           = value_type* const*;
            using reference         = value_type* const&;

            Tag tag_;
            uintptr_t ptr_;
            std::deque<node*> stack_;

            constexpr iterator() noexcept = default;
            constexpr iterator(value_type* d) noexcept
                : tag_(Tag::Uniq)
                , ptr_(std::bit_cast<uintptr_t>(d)) {}
            constexpr iterator(value_type* const* elems) noexcept
                : tag_(Tag::Data)
                , ptr_(std::bit_cast<uintptr_t>(elems)) {}
            constexpr iterator(node* n) noexcept
                : tag_(Tag::Node)
                , ptr_(std::bit_cast<uintptr_t>(n)) {}

            iterator& clear() noexcept {
                tag_ = Tag::Null;
                ptr_ = 0;
                return *this;
            }

            constexpr bool operator==(iterator other) const noexcept { return this->tag_ == other.tag_ && this->ptr_ == other.ptr_; }
            constexpr bool operator!=(iterator other) const noexcept { return this->tag_ != other.tag_ || this->ptr_ != other.ptr_; }

            constexpr value_type operator*() const noexcept {
                switch (tag_) {
                    case Tag::Uniq:  return  std::bit_cast<value_type*>(ptr_);
                    case Tag::Array: return *std::bit_cast<value_type* const*>(ptr_);
                    case Tag::Node:  return  std::bit_cast<node*>(ptr_)->val_;
                }
            }

            constexpr iterator& operator++() noexcept {
                switch (tag_) {
                    case Tag::Uniq:  return clear();
                    case Tag::Array: return ptr_ = std::bit_cast<uintptr_t>(std::bit_cast<value_type* const*>(ptr_) + 1), *this;
                    case Tag::Node: {
                        auto n = std::bit_cast<node*>(ptr_);
                        if (n->has_right()) {
                            n=n->right();
                            stack_.push_back(n);
                            while (n->has_left()) {
                                n = n->left();
                                stack_.push_back(n);
                            }
                            stack_.pop_back();
                            return *this;
                        }
                        if (stack_.empty()) return clear();
                        n=stack_.back();
                        stack_.pop_back();
                        return *this;
                    }
                }
            }
            constexpr iterator operator++(int) noexcept {
                auto res = *this;
                this->operator++();
                return res;
            }
            constexpr pointer operator->() const noexcept { return this->operator*(); }
        };
    };
 
    setmap create() {
        return {};
    }

    setmap insert(setmap sm, value_type val) {
        if (auto u = sm.isa_uniq()) {
            if (val == *u) return {*u};

            auto [data, state] = allocate(2);
            if (key(val) < key(*u)) data->arr_[0] = val, data->arr_[1] = *u;
            else                    data->arr_[0] = *u,  data->arr_[1] = val;

            return setmap(data);
        }

        if (auto src = sm.isa_data()) {
            auto size = src->size_;
            if (size + 1 <= ARR_LIM) {
                auto [dst, state] = allocate(size + 1);

                // copy over and insert new element
                bool ins = false;
                for (auto si = src->begin(), di = dst->begin(), se = src->end(); si != se || !ins; ++di) {
                    if (key(val) == key((*si))) { // already here
                        if constexpr (is_set()) {
                            arena_->deallocate(state);
                            return sm;
                        }
                        else {
                            if ((*si).second==val.second) {
                                arena_->deallocate(state);
                                return sm;
                            }
                            else {
                                *di = val, ins = true, ++si;
                                //would like to dealloc last elem of array but how :(
                                continue;
                            }
                        }
                    }
                    if (!ins && (si == se || key(val) < key(*si)))
                        *di = val, ins = true;
                    else
                        *di = *si++;
                }

                return setmap(dst);
            } else { // we need to switch from Data to Node
                auto [dst, state] = allocate(size + 1);

                // copy over
                auto di = dst->begin();
                for (auto si = src->begin(), se = src->end(); si != se; ++si, ++di) {
                    if (key(val) == key(*si)) { // already here
                        if constexpr (is_set()) {
                            arena_.deallocate(state);
                            return sm;
                        }
                        else {
                            if (val.second == (*si).second) {
                                arena_.deallocate(state);
                                return sm;
                            }
                        }

                    }

                    *di = *si;
                }
                *di = val; // put new element at last into dst->elems

                // sort in ascending tids but 0 goes last
                std::sort(dst->begin(), di);

                return setmap(build_balanced(dst));
            }
        }

        if (auto n = sm.isa_node()) {
            if (n->contains(val)) return sm;
            return setmap(insert(n, val));
        }

        return {val}; //just so compiler doesnt complain, should be unreachable
    }

    node* insert(node* n, value_type val) {
        if (n==nullptr) return make_node(val).first;

        if (key(n->val_)==key(val)) { //key already exists
            if constexpr (is_set) return n; 
            else if (n->val().second==val.second) return n; 

            //map case: insert new value for existing key in the middle of the tree
            return make_node(n->left(),n->right(),val).first;
        }

        node *l, *r;
        if (!Compare{}(key(val),n->key())) {
            l = n->left();
            r = insert(n->right(), val);
        }
        else {
            l = insert(n->left(), val);
            r = n->right();
        }

        if (l==n->left()&&r==n->right()) return n; //if both children are the same dont create new node
        else return balance_node(make_node(l,r,n->val()).first);
    }

    setmap merge(setmap sm1, setmap sm2);



    struct freezer {
        using sm = setmap;
        sm sm_;
        sm* ptr_;

        freezer(sm* s) :
            sm_(*s), ptr_(s) {}

        ~freezer() {
            *ptr_ = sm_;
        }
    };



};





}