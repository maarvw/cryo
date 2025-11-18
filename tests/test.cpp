#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include<doctest/doctest.h>
#include<cryo/set.h>
#include"cryo/rbtree.h"
#include<iostream>
using namespace std;

TEST_CASE("Arena") {
    cryo::Set set(23);
    CHECK(set.key() == 23);
}

TEST_CASE("adding-stuff") {
    cryo::rbtree knecht1 = cryo::rbtree<int>();
    cryo::rbtree knecht2 = knecht1.add(1);
    CHECK(knecht1.getSize() == 0);
    CHECK(knecht2.getSize() == 1);
    CHECK(knecht2[0] == 1);

    cryo::rbtree<int> knechte[50];
    for (int i=1; i<50; i++){
        knechte[i]=knechte[i-1].add(i);
    }

    cryo::rbtree knecht3 = knechte[45];

    CHECK(knecht3[25] == 26);
    cryo::rbtree knecht4 = knecht3.insert(25, 1);
    CHECK(knecht3[25] == 26);
    CHECK(knecht4[25] == 1);
}

TEST_CASE("constructor-list") {
    cryo::rbtree<int> k1({1, 2,3,4,5,6,7,8});
    cout<<k1[7]<<endl;
    CHECK(k1[7] == 8);
    CHECK(k1.getSize() == 8);
}