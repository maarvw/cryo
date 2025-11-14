#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
namespace cryo {

template<typename T>
struct rbtree {
    static const int B = 5;
    static const int M = 1 << B;

    struct node {
        node* inner[M];
        T leaf[M];
    };

    node* root;
    size_t size;
    int shift;
    int capacity;


    rbtree() {
        size = 0;
        root = node {NULL, new T[M]};
        shift = 0; //bei erhöhung der höhe +B
        capacity = M;
    }

    rbtree(node* r, size_t s, int sf, int cap) {
        r=root, size=s, shift=sf, capacity=cap;
    }

    void add_primitive(T elem) {
        size++;
        if (size>=capacity) { //new root
            node newroot = {new node*[M], NULL}; 
            newroot.inner[0]=root;
            root=&newroot;
            shift+=B;
            capacity*=M;
        }
        node* cur = root;
        int i = size;
        int s = shift;
        while (s != 0) {
            if (cur->inner[(i>>s)%M]==NULL) {
                cur->inner[(i>>s)%M] = (s == B ? new node* {NULL, new T[M]} : new node* {new node*[M], NULL});
            }
            cur = cur->inner[(i >> s) % M];
            s -= B;
        }
        cur->leaf[i%M]=elem;
    }

    rbtree add(T elem) {
        if (size+1 >= capacity) { //tree full, need to expand depth
            int s = shift;
            node newroot = {new node*[M], NULL}; 
            newroot.inner[0]=root;
            newroot.inner[1]= (s == 0 ? new node* {NULL, new T[M]} : new node* {new node*[M], NULL});
            node* newnode = &newroot.inner[1]; //newroot remains "root" of new subtree
            while (s!=0) {
                newnode->inner[0] = (s == B ? new node* {NULL, new T[M]} : new node* {new node*[M], NULL});
                newnode=newnode[0];
                s -= B;
            }
            newnode->leaf[0]=elem;
            return rbtree(&newroot, size+1, shift+B, capacity*M);
        }
        node* newroot = add_help(elem, root, shift, size+1);
        return rbtree(newroot, size+1, shift, capacity);
    }
    
    node* add_help(T elem, node* cur, int s, int i) {
        if (s==0) {
            //copy & change leaf
            node newleaf = {NULL, new T[M]};
            std::copy_n(cur->leaf, cur->leaf+M, newleaf.leaf);
            newleaf.leaf[i%M]=elem;
            return &newleaf;
        }
        node newinner = {new node*[M], NULL};
        std::copy_n(cur->inner, cur->inner+M, newinner.inner);
        if (cur->inner[(i>>s)%M]==NULL) { //need to build new subtree of inners
            node* newnode = &newinner; //newinner remains "root" of new subtree
            while (s!=0) {
                newnode->inner[(i>>s)%M] = (s == B ? new node* {NULL, new T[M]} : new node* {new node*[M], NULL});
                newnode=newnode[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                s -= B;
            }
            newnode->leaf[0]=elem;
            return &newinner;
        }
        newinner.inner[(i>>s)%M]=add_help(elem, cur->inner[(i>>s)%M], s-B, i);
        return &newinner;
    }

    T& get(size_t i) {
        if (i<=size) 
            throw "dummer hurensohn";
        node* cur = root;
        int s = shift;
        while (s != 0) {
            cur = cur->inner[(i >> s) % M];
            s -= B;
        }
        return cur->leaf[i%M];
    }

    const T& operator[] (size_t i) {
        return get(i);
    }

};



template<typename T>
class Set {
public:

    Set() = default;

    Set(const T& key)
        : key_(key) {}

    T key() const { return key_; }

private:
    T key_;
};

}
