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

TEST_CASE("string-add-primitive") {
    cryo::rbtree knecht1 = cryo::rbtree<string>("hallo");
    //knecht1.add_primitive("hi");
}

TEST_CASE("structs") {
    struct point {
        int x, y;
    };
    cryo::rbtree<point> punkte = cryo::rbtree<point>();
    punkte.add({1,2});
}

TEST_CASE("char-pointers"){
    cryo::rbtree knecht1 = cryo::rbtree<char*>();
    char k = 'a';
    cryo::rbtree knecht2 = knecht1.add(&k);
}

TEST_CASE("string-add-persistent") {
    cryo::rbtree knecht1 = cryo::rbtree<string>("hallo");
    cryo::rbtree knecht2 = knecht1.add("hi");
    CHECK(knecht2[0]=="hallo");
}

TEST_CASE("structs-with-string") {
    struct point2 {
        int x, y;
        string s;
    };
    cryo::rbtree<point2> punkte = cryo::rbtree<point2>({1,2, "hi"});
    punkte.add({3,4, "hii"});
}

TEST_CASE("string-add-persistent-insert") {
    cryo::rbtree knecht1 = cryo::rbtree<string>({"hallo","a", "b", "c"});
    cryo::rbtree knecht2 = knecht1.insert(1, "kek");
}

TEST_CASE("32 strings") {
    cryo::rbtree knecht = cryo::rbtree<string>({"s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s"});
    knecht.insert(17, "a");
}


TEST_CASE("vectors") {
    cryo::rbtree<vector<int>> k1 = cryo::rbtree<vector<int>>({1,2,3,4,5});
    cryo::rbtree<vector<int>> k2 = k1.add({5,4,2});
    CHECK(k2[1][0]==5);
}