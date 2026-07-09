#pragma once

#include <cstdint>
#include <fe/arena.h>
#include <deque>
#include <stdexcept>
#include <type_traits>


using arena = fe::Arena;


namespace cryo {

template<typename K, typename V = void, size_t ARR_LIM = 16, typename Compare = std::less<K>>
class setmaps {
    public:
    using arena = fe::Arena;

    using value_type = std::conditional_t<std::is_void_v<V>, K, std::pair<K, V>>;
    using mapped_type = V;

    arena arena_;

    static constexpr bool is_set() {return  std::is_void_v<V>; }
    static constexpr bool is_map() {return !std::is_void_v<V>; }

    static constexpr K key(value_type val) {
        if constexpr (is_set()) { return val;       }
        else                    { return val.first; }
    }

    struct node {
        node* left_;
        node* right_;
        value_type val_;
        int height_ = 1;
        int balance_ = 0;

        constexpr node(value_type val)  noexcept
            : val_(val)
            , left_(nullptr)
            , right_(nullptr) {}

        constexpr node(node* l, node* r, value_type val) noexcept
            : left_(l)
            , right_(r)
            , val_(val) {
                recalculate_balance();
        }

        ~node() {}

        node* left()     const { return left_;  }
        node* right()    const { return right_; }
        value_type val() const { return val_;   }
        K get_key()      const { return key(val_); }

        size_t size() const {
            size_t s = 1;
            if (has_left() ) s += left_ ->size();
            if (has_right()) s += right_->size();
            return s;
        }

        void set_left (node* n) { left_  = n; recalculate_balance(); }
        void set_right(node* n) { right_ = n; recalculate_balance(); }

        bool has_left () const { return left_  != nullptr; }
        bool has_right() const { return right_ != nullptr; }

        void recalculate_balance() {
            int rh = (right_ != nullptr ? right_->height_ : 0);
            int lh = (left_  != nullptr ? left_ ->height_ : 0);
            height_  = 1+std::max(lh,rh);
            balance_ = rh-lh;
        }

        // static node* left_rotate(node* n) {
        //     node* r =  n->right_;
        //     node* rl = r->left_;
        //     n->set_right(rl);
        //     r->set_left(n);
        //     return r;
        // }
        // static node* right_rotate(node* n) {
        //     node* l  = n->left_;
        //     node* lr = l->right_;
        //     n->set_left(lr);
        //     l->set_right(n);
        //     return l;
        // }
        // static node* balance_node(node* n) {
        //     int bal = n->balance_;
        //     if (bal<-1) {
        //         return right_rotate(n);
        //     }
        //     else if (bal>1) {
        //         return left_rotate(n);
        //     }
        //     return n;
        // }

        bool contains(K val) const {
            if (val == key(val_)) return true;
            if (val <  key(val_)) {
                if (!has_left()) return false;
                else            return left_->contains(val);
            }
            if (val >  key(val_)) {
                if (!has_right()) return false;
                else             return right_->contains(val);
            }
            return false;
        }


    };

    node* build_balanced(value_type* sorted, size_t lo, size_t hi) {
        if (lo >= hi) return nullptr;
        size_t mid = lo + (hi - lo) / 2;

        value_type val = sorted[mid];

        node* left  = build_balanced(sorted, lo, mid);
        node* right = build_balanced(sorted, mid + 1, hi);

        return make_node(left, right, val).first;
    }

    struct arr {
        size_t size_ = 0;
        value_type* arr_;

        constexpr arr() noexcept = default;
        constexpr arr(size_t size, value_type* arr) noexcept
            : size_(size)
            , arr_(arr) {}

        ~arr() {}

        constexpr value_type* begin() noexcept { return arr_; }
        constexpr value_type* end() noexcept { return arr_ + size_; }
        constexpr value_type const* begin() const noexcept { return arr_; }
        constexpr value_type const* end() const noexcept { return arr_ + size_; }
    };

    std::pair<arr*, arena::State> allocate(size_t size) {
        auto bytes = sizeof(arr) + size * sizeof(value_type);
        auto state = arena_.state();
        auto buff  = arena_.allocate(bytes, alignof(arr));
        // auto data  = new (buff) arr(size);
        // return {data, state};
        value_type* data_ptr = reinterpret_cast<value_type*>(
            static_cast<char*>(buff) + sizeof(arr)
        );
        auto a = new (buff) arr(size, data_ptr);
        return {a, state};
    }

    std::pair<node*, arena::State> make_node(node* l, node* r, value_type val) {
        auto bytes = sizeof(node);
        auto state = arena_.state();
        auto buff  = arena_.allocate(bytes, alignof(node));
        auto data = new (buff) node(l, r, val);
        return {data, state};
    }
    std::pair<node*, arena::State> make_node(value_type val) {
        return make_node(nullptr, nullptr, val);
    }

    std::pair<value_type*, arena::State> alloc_uniq(value_type v) {
        auto bytes = sizeof(value_type);
        auto state = arena_.state();
        auto buff = arena_.allocate(bytes, alignof(value_type));
        auto data = new (buff) value_type();
        *data = v;
        return {data, state};
    }


    class setmap {
        public:
        uintptr_t data_ = 0;

        enum class Tag : uintptr_t { Null, Uniq, Array, Node };
        constexpr Tag tag() const noexcept { return Tag(data_ & uintptr_t(0b11)); }

        template<class T>
        constexpr T* ptr() const noexcept {
            return std::bit_cast<T*>(data_ & ~uintptr_t(0b11));
        }

        constexpr value_type* isa_uniq() const noexcept { return tag() == Tag::Uniq  ? ptr<value_type>() : nullptr; }
        constexpr arr*        isa_arr()  const noexcept { return tag() == Tag::Array ? ptr<arr       >() : nullptr; }
        constexpr node*       isa_node() const noexcept { return tag() == Tag::Node  ? ptr<node      >() : nullptr; }

        constexpr setmap(const setmap&) noexcept = default;
        constexpr setmap(setmap&&) noexcept      = default;
        constexpr setmap() noexcept              = default; ///< Null setmap
        constexpr setmap(value_type* d) noexcept
            : data_(uintptr_t(d) | uintptr_t(Tag::Uniq)) {} ///< Uniq setmap.
        constexpr setmap(const arr* data) noexcept
            : data_(uintptr_t(data) | uintptr_t(Tag::Array)) {} ///< Array setmap.
        constexpr setmap(node* node) noexcept
            : data_(uintptr_t(node) | uintptr_t(Tag::Node)) {
                assert(node != nullptr);
                assert((uintptr_t(node) & 0b11) == 0);  // must be aligned
        } ///< Node setmap.
        constexpr setmap(uintptr_t data) noexcept
            : data_(data) {}

        constexpr setmap& operator=(const setmap& other) noexcept {
            data_ = other.data_;
            return *this;
        }

        constexpr uintptr_t data() const noexcept { return data_; }

        constexpr size_t size() const noexcept {
            if (isa_uniq()) return 1;
            if (auto d = isa_arr()) return d->size_;
            if (auto n = isa_node()) return n->size();
            return 0; // empty
        }

        constexpr bool empty() const noexcept {
            return data_ == 0;
        }

        bool contains(K d) const noexcept {
            if (auto u = isa_uniq()) return d == key(*u);

            if (auto data = isa_arr()) {
                for (auto e : *data)
                    if (d == key(e)) return true;
                return false;
            }

            if (auto n = isa_node()) return n->contains(d);

            return false;
        }

        template<typename U = V,  typename = std::enable_if_t<!std::is_void_v<U>>>
        const V operator[](K k) const {
            //static_assert(!std::is_void_v<V>, "operator[] not supported for void");
            // Missing key yields a default-constructed value (0 / nullptr); guard with contains() if you
            // need to distinguish "absent" from "mapped to default".
            if (auto u = isa_uniq()) {
                if (key(*u) == k) return u->second;
                return V{};
            }
            if (auto a = isa_arr()) {
                for (auto e : *a)
                    if (key(e) == k) return e.second;
                return V{};
            }
            if (auto n = isa_node()) {
                while (n!=nullptr) {
                    if (n->get_key() == k) return n->val().second;
                    if (n->get_key() > k) n = n->left();
                    else                  n = n->right();
                }
                return V{};
            }
            return V{};
        }


        struct iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = setmaps::value_type;
            using pointer           = value_type* const*;
            using reference         = value_type* const&;

            Tag tag_;
            uintptr_t ptr_;
            std::deque<node*> stack_;

            constexpr iterator() noexcept = default;
            constexpr iterator(value_type* d) noexcept
                : tag_(Tag::Uniq)
                , ptr_(std::bit_cast<uintptr_t>(d)) {}
            constexpr iterator(arr* a) noexcept
                : tag_(Tag::Array)
                , ptr_(std::bit_cast<uintptr_t>(a->arr_)) {}
            constexpr iterator(arr* a, size_t size) noexcept //for end
                : tag_(Tag::Array)
                , ptr_(std::bit_cast<uintptr_t>(a->arr_+size)) {}
            constexpr iterator(node* n) noexcept
                : tag_(Tag::Node)
                , ptr_(std::bit_cast<uintptr_t>(n)) {
                    node* cur = n;
                    while (cur->has_left()) {
                        stack_.push_back(cur);
                        cur = cur->left();
                    }
                }

            iterator& clear() noexcept {
                tag_ = Tag::Null;
                ptr_ = 0;
                return *this;
            }

            constexpr bool operator==(iterator other) const noexcept { return this->tag_ == other.tag_ && this->ptr_ == other.ptr_; }
            constexpr bool operator!=(iterator other) const noexcept { return this->tag_ != other.tag_ || this->ptr_ != other.ptr_; }

            constexpr value_type operator*() const noexcept {
                switch (tag_) {
                    case Tag::Uniq:  return *std::bit_cast<value_type*>(ptr_);
                    case Tag::Array: return *std::bit_cast<value_type const*>(ptr_);
                    case Tag::Node:  return  std::bit_cast<node*>(ptr_)->val_;
                    default:         return {};
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
                    default: return clear();
                }
            }
            constexpr iterator operator++(int) noexcept {
                auto res = *this;
                this->operator++();
                return res;
            }
            constexpr pointer operator->() const noexcept { return this->operator*(); }
        };

        constexpr iterator begin() const noexcept {
            if (auto u = isa_uniq()) return {u};
            if (auto d = isa_arr()) return {d};
            if (auto n = isa_node()) return {n};
            return {};
        }

        constexpr iterator end() const noexcept {
            if (auto data = isa_arr()) return {data, data->size_};
            return {};
        }



    };

    setmap create() {
        return {};
    }

    setmap create(value_type val) {
        return {alloc_uniq(val).first};
    }

    node* left_rotate(node* n) {
        node* r  = n->right_;
        node* rl = r->left_;
        node* new_n = make_node(n->left_, rl, n->val_).first;
        node* new_r = make_node(new_n, r->right_, r->val_).first;
        return new_r;
    }

    node* right_rotate(node* n) {
        node* l  = n->left_;
        node* lr = l->right_;
        node* new_n = make_node(lr, n->right_, n->val_).first;
        node* new_l = make_node(l->left_, new_n, l->val_).first;
        return new_l;
    }

    node* balance_node(node* n) {
        int bal = n->balance_;
        if (bal < -1) {
            return right_rotate(n);
        } else if (bal > 1) {
            return left_rotate(n);
        }
        return n;
    }

    setmap insert(setmap sm, value_type val) {
        if (auto u = sm.isa_uniq()) {
            if (key(val) == key(*u)) {
                if constexpr (is_set()) return sm;
                else if (val.second == (*u).second) return sm;
                else return {alloc_uniq(val).first};
            }

            auto [data, state] = allocate(2);
            if (key(val) < key(*u)) data->arr_[0] = val, data->arr_[1] = *u;
            else                    data->arr_[0] = *u,  data->arr_[1] = val;

            return setmap(data);
        }

        if (auto src = sm.isa_arr()) {
            auto size = src->size_;

            // If the key already exists, update the value in place (or no-op) - never grow.
            // Handling updates here keeps the grow/convert paths below dealing only with *new* keys,
            // which avoids both dropping map updates and creating duplicate keys during Array->Node.
            for (auto si = src->begin(), se = src->end(); si != se; ++si) {
                if (key(val) == key(*si)) {
                    if constexpr (is_set()) {
                        return sm;
                    } else {
                        if ((*si).second == val.second) return sm;
                        auto [dst, state] = allocate(size);
                        auto di = dst->begin();
                        for (auto s = src->begin(); s != se; ++s, ++di) *di = (key(*s) == key(val)) ? val : *s;
                        return setmap(dst);
                    }
                }
            }

            // key is new from here on
            if (size + 1 <= ARR_LIM) {
                auto [dst, state] = allocate(size + 1);

                // copy over and insert new element in sorted position
                bool ins = false;
                for (auto si = src->begin(), di = dst->begin(), se = src->end(); si != se || !ins; ++di) {
                    if (!ins && (si == se || key(val) < key(*si)))
                        *di = val, ins = true;
                    else
                        *di = *si++;
                }

                return setmap(dst);
            } else { // we need to switch from Array to Node
                auto [dst, state] = allocate(size + 1);

                // copy over, then append the new element
                auto di = dst->begin();
                for (auto si = src->begin(), se = src->end(); si != se; ++si, ++di) *di = *si;
                *di = val; // put new element at last into dst->arr_

                std::sort(dst->begin(), dst->end());

                std::vector<value_type> tmp(dst->begin(), dst->end());
                return setmap(build_balanced(tmp.data(), 0, tmp.size()));
            }
        }

        if (auto n = sm.isa_node()) {
            // insert() handles new keys, no-ops, and value updates for existing keys.
            return setmap(insert(n, val));
        }

        return {alloc_uniq(val).first};
    }

    node* insert(node* n, value_type val) {
        if (n==nullptr) return make_node(val).first;

        if (key(n->val_)==key(val)) { //key already exists
            if constexpr (is_set()) return n;
            else if (n->val().second==val.second) return n;

            //map case: insert new value for existing key in the middle of the tree
            return make_node(n->left(),n->right(),val).first;
        }

        node *l, *r;
        if (!Compare{}(key(val),n->get_key())) {
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

    friend void swap(setmaps& a, setmaps& b) noexcept {
        using std::swap;
        swap(a.arena_, b.arena_); // ADL -> fe::swap(Arena&, Arena&); handles into it stay valid
    }

};





}
