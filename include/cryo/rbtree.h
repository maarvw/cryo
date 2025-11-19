#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <iostream>
#include "../../external/fe/include/fe/arena.h"
namespace cryo {

/*radix balanced tree with persistent functionality, 
  serves as underlying datastructure for containers*/
template<typename T>
class rbtree {

private:
    /*amount of branching bits used for the index bitshifts*/
    static const int B = 5;
    /*amount of array elements per node (power of 2)*/
    static const int M = 1 << B;

    /*arena used for the arena allocation*/
    static fe::Arena arena;
    
    

    /*node of the rbtree*/
    struct node {
        /*allocator for the leafs*/
        static fe::Arena::Allocator<T> leafallocator;
        /*allocator for the inner nodes*/
        static fe::Arena::Allocator<node*> innerallocator;

        /*array for the leaf data elements (only for leafs)*/
        T* leaf;
        /*array of pointers to child nodes (only for inner nodes)*/
        node** inner;
        
        /*default constructor using arena allocators*/ 
        node() : leaf(leafallocator.allocate(M)), inner(innerallocator.allocate(M)) {}
    };

    /*root node of the tree, unique to every persistent "copy"*/
    node* root;
    /*numer of elements in the tree*/
    size_t size;
    /*bitshift amount at the root level. power of 2.*/
    int shift;
    /*current capacity of the tree. multiple of M. if full, add() creates new layer*/
    int capacity;

    /*initialises empty tree to be used by constructors*/
    void empty_init() {
        size = 0;
        root = new node;
        shift = 0; //bei erhöhung der höhe +B
        capacity = M;
    }

    /*contructor for new tree, only intended to be used by update functions*/
    rbtree(node* r, size_t s, int sf, int cap) {
        root=r, size=s, shift=sf, capacity=cap;
    }

    /*adds element in non-persistent way, not intended to be used outside of constructors*/
    void add_primitive(T elem) {
        size++;
        std::cout<<"new elem, new size="<<size<<std::endl;
        if (size>=capacity) { //new root
            std::cout<<"new cap"<<std::endl;
            int s = shift;
            node* newroot = new node; 
            newroot->inner[0]=root;
            newroot->inner[1]= new node;
            node* newnode = newroot->inner[1]; //newroot remains "root" of new subtree
            while (s!=0) {
                newnode->inner[0] = new node;
                newnode=newnode->inner[0];
                s -= B;
            }
            newnode->leaf[0]=elem;
            root = newroot;
            shift+=B;
            capacity*=M;
            return;
        }
        node* cur = root;
        int i = size-1;
        int s = shift;
        while (s != 0) {
            if (cur->inner[(i>>s)%M]==NULL) {
                cur->inner[(i>>s)%M] = new node;
            }
            cur = cur->inner[(i >> s) % M];
            s -= B;
        }
        cur->leaf[i%M]=elem;
    }

    
    /*recursive helper function adding an element at a specific index in the tree,
      expanding the tree if necessary. creates new nodes and returns a new root element
      for the new persistent "copy"*/
    node* insert_at_index(T elem, node* cur, int s, size_t i) {
        if (s==0) {
            std::cout<<"insert base case"<<std::endl;
            //copy & change leaf
            node* newleaf = new node;
            // std::cout<<"copying"<<std::endl;
            // std::copy_n(cur->leaf, M, newleaf->leaf);
            // std::cout<<"changing"<<std::endl;
            // newleaf->leaf[i%M]=elem;
            for (int j = 0; j < M; j++) {
                if (j == i % M) {
                    std::cout<<"j=="<<i<<std::endl;
                    newleaf->leaf[j] = elem;  // Assignment instead of copy
                } else {
                    std::cout<<"j="<<j<<std::endl;
                    newleaf->leaf[j] = cur->leaf[j];  // Copy existing elements
                }
            }
            std::cout<<"loop over"<<std::endl;
            std::cout<<"returning"<<std::endl;
            return newleaf;
        }
        std::cout<<"add with s="<<s<<std::endl;
        node* newinner = new node;
        std::copy_n(cur->inner, M, newinner->inner);
        if (cur->inner[(i>>s)%M]==NULL) { //need to build new subtree of inners
            node* newnode = newinner; //newinner remains "root" of new subtree
            while (s!=0) {
                newnode->inner[(i>>s)%M] = new node;
                newnode=newnode->inner[(i>>s)%M]; //(i>>s)%M should always be 0 in this loop but just in case
                s -= B;
            }
            newnode->leaf[0]=elem;
            return newinner;
        }
        newinner->inner[(i>>s)%M]=insert_at_index(elem, cur->inner[(i>>s)%M], s-B, i);
        return newinner;
    }

    
public:

    /*default constructor for empty tree*/
    rbtree() {
        empty_init();
    }

    /*single element constructor*/
    rbtree(T elem) {
        empty_init();
        add_primitive(elem);
    }

    /*constructor for initializer list*/
    rbtree(std::initializer_list<T> elems) {
        empty_init();
        for (T elem : elems)
            add_primitive(elem);
    }

    /*adds new element to the tree, expanding the tree if necessary.
      returns a persistent "copy" of the previous tree*/
    rbtree add(T elem) {
        if (size+1 >= capacity) { //tree full, need to expand depth
            std::cout<<"add base case"<<std::endl;
            int s = shift;
            node* newroot = new node; 
            newroot->inner[0]=root;
            newroot->inner[1]= new node;
            node* newnode = newroot->inner[1]; //newroot remains "root" of new subtree
            while (s!=0) {
                newnode->inner[0] = new node;
                newnode=newnode->inner[0];
                s -= B;
            }
            newnode->leaf[0]=elem;
            return rbtree(newroot, size+1, shift+B, capacity*M);
        }

        std::cout<<"add recursive case"<<std::endl;
        node* newroot = insert_at_index(elem, root, shift, size);
        return rbtree(newroot, size+1, shift, capacity);
    }

    /*adds whole list of elements at once but no fancy transience stuff (yet)*/
    rbtree add(std::initializer_list<T> elems) {
        rbtree newtree;
        for (T elem : elems) {
            newtree = newtree.add(elem);
        }
        return newtree;
    }

    /*inserts an element at a specific (previously existing) index in the tree, 
      overwriting the data at that index. returns a persistent "copy" of the previous tree.*/
    rbtree insert(size_t i, T elem) {
        if (i>=size)
            throw std::runtime_error("dummer hurensohn");
        node* newroot = insert_at_index(elem, root, shift, i);
        return rbtree(newroot, size, shift, capacity);
    }

    /*returns element at specific index, making no changes.
      equivalent to the [] operator*/
    const T& get(size_t i) {
        if (i>=size) 
            throw std::runtime_error("dummer hurensohn"); //vielleicht nochmal ändern lol
        node* cur = root;
        int s = shift;
        while (s != 0) {
            cur = cur->inner[(i >> s) % M];
            s -= B;
        }
        return cur->leaf[i%M];
    }

    /*returns element at specific index, making no changes.
      equivalent to the get() function*/
    const T& operator[] (size_t i) {
        return get(i);
    }

    /*returns number of elements in the tree*/
    int getSize() {
        return size;
    }
};


//muss damit richtig funktioniert sagt ki
template<typename T>
fe::Arena rbtree<T>::arena;

template<typename T>
fe::Arena::Allocator<T> rbtree<T>::node::leafallocator = rbtree<T>::arena.allocator<T>();

template<typename T>
fe::Arena::Allocator<typename rbtree<T>::node*> rbtree<T>::node::innerallocator = rbtree<T>::arena.allocator<typename rbtree<T>::node*>();
}