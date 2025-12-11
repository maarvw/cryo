// #pragma once
// #include "rbtree.h"
// #include <initializer_list>

// namespace cryo {

// template<typename T>
// class vector {

// private:

//     /*the tree used internally*/
//     rbtree<T>* tree;

//     vector(rbtree<T> tree) :
//         tree(&tree) {}

//     vector(rbtree<T>* tree) :
//         tree(tree) {}
    

// public:

//     vector() :
//         tree(new rbtree<T>()) {}
    

//     vector(T elem) :
//         tree(new rbtree<T>(elem)) {}

//     vector(std::initializer_list<T> elems) :
//         tree(new rbtree<T>(elems)) {}
    


//     vector push_back(T elem) {
//         rbtree<T> t = tree->add(elem);
//         return new vector(t);
//         //return new vector(tree->add(elem));
//     }

//     vector insert(size_t i, T elem) {
//         return new vector(tree->insert(i, elem));
//     }

//     const T& get(size_t i) {
//         return tree->get(i);
//     }

//     const T& operator[](size_t i) {
//         return get(i);
//     }

//     size_t size(){
//         return tree->getSize();
//     }

// };

// }