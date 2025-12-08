#include <stdexcept>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include<doctest/doctest.h>
#include<cryo/set.h>
#include"cryo/trees.h"
//#include"cryo/vector.h"
#include<iostream>
using namespace std;

#if 0
TEST_CASE("Arena") {
    cryo::Set set(23);
    CHECK(set.key() == 23);
}

TEST_CASE("adding-stuff") {
    cryo::trees knecht1 = cryo::trees<int>();
    cryo::rbtrees knecht2 = knecht1.add(1);
    CHECK(knecht1.getSize() == 0);
    CHECK(knecht2.getSize() == 1);
    CHECK(knecht2[0] == 1);

    cryo::rbtrees<int> knechte[50];
    for (int i=1; i<50; i++){
        knechte[i]=knechte[i-1].add(i);
    }

    cryo::rbtrees knecht3 = knechte[45];

    CHECK(knecht3[25] == 26);
    cryo::rbtrees knecht4 = knecht3.insert(25, 1);
    CHECK(knecht3[25] == 26);
    CHECK(knecht4[25] == 1);
}

TEST_CASE("constructor-list") {
    cryo::rbtrees<int> k1({1, 2,3,4,5,6,7,8});
    cout<<k1[7]<<endl;
    CHECK(k1[7] == 8);
    CHECK(k1.getSize() == 8);
}

TEST_CASE("string-add-primitive") {
    cryo::rbtrees knecht1 = cryo::rbtrees<string>("hallo");
    //knecht1.add_primitive("hi");
}

TEST_CASE("structs") {
    struct point {
        int x, y;
    };
    cryo::rbtrees<point> punkte = cryo::rbtrees<point>();
    punkte.add({1,2});
}

TEST_CASE("char-pointers"){
    cryo::rbtrees knecht1 = cryo::rbtrees<char*>();
    char k = 'a';
    cryo::rbtrees knecht2 = knecht1.add(&k);
}

TEST_CASE("string-add-persistent") {
    cryo::rbtrees knecht1 = cryo::rbtrees<string>("hallo");
    cryo::rbtrees knecht2 = knecht1.add("hi");
    CHECK(knecht2[0]=="hallo");
}

TEST_CASE("structs-with-string") {
    struct point2 {
        int x, y;
        string s;
    };
    cryo::rbtrees<point2> punkte = cryo::rbtrees<point2>({1,2, "hi"});
    punkte.add({3,4, "hii"});
}

TEST_CASE("string-add-persistent-insert") {
    cryo::rbtrees knecht1 = cryo::rbtrees<string>({"hallo","a", "b", "c"});
    cryo::rbtrees knecht2 = knecht1.insert(1, "kek");
}

TEST_CASE("32 strings") {
    cryo::rbtrees knecht = cryo::rbtrees<string>({"s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s","s"});
    knecht.insert(17, "a");
}


TEST_CASE("std::vectors") {
    cryo::rbtrees<vector<int>> k1 = cryo::rbtrees<vector<int>>({1,2,3,4,5});
    cryo::rbtrees<vector<int>> k2 = k1.add({5,4,2});
    CHECK(k2[1][0]==5);
}


TEST_CASE("vector") {
    cryo::vector<int> k0 = cryo::vector<int>();
    cryo::vector<int> k1 = k0.push_back(42);
    cryo::vector<int> k2 = k1.push_back(43);
    cryo::vector<int> k3 = k2.push_back(44);
    cryo::vector<int> k4 = k3.insert(1, 52);
    CHECK(k0.size()==0);
    CHECK(k1.size()==1);
    CHECK(k2.size()==2);
    CHECK(k3.size()==3);
    CHECK(k3[1]==43);
    CHECK(k4[1]==52);
}
#endif


TEST_CASE("neu") {
    cryo::trees meister = cryo::trees<int>();
    cryo::trees<int>::tree knecht1 = meister.get();
    cryo::trees<int>::tree knecht2 = meister.add(knecht1, 42);
    CHECK(knecht2[0]==42);
    CHECK(knecht1.getSize()==0);
    CHECK(knecht2.getSize()==1);
}

TEST_CASE("id matching"){
    using cryo::trees;
    trees meister1 = trees<int>();
    trees<int>::tree knecht1 = meister1.get();
    trees<int>::tree knecht2 = meister1.add(knecht1, 42);
    trees meister2 = trees<int>();
    trees<int>::tree knecht3 = meister2.get();
    trees<int>::tree knecht4 = meister2.add(knecht3, 24);
    try {
        meister2.add(knecht1,67);
    } catch (runtime_error e) {
        cout<<e.what()<<endl;
    }
}

TEST_CASE("constructor-list") {
    cryo::trees<int> k({1, 2,3,4,5,6,7,8});
    cryo::trees<int>::tree k1 = k.get();
    CHECK(k1[7] == 8);
    CHECK(k1.getSize() == 8);
}

TEST_CASE("using") {
    using cryo::trees;
    trees<int> k({1, 2,3,4,5,6,7,8});
    trees<int>::tree k1 = k.get();
    CHECK(k1[7] == 8);
    CHECK(k1.getSize() == 8);
    cout<<"hallo"<<endl;
}