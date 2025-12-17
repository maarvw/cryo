#include <iterator>
#include <stdexcept>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include<doctest/doctest.h>
#include<cryo/set.h>
#include"cryo/trees.h"
#include<iostream>
using std::cout;
using std::endl;
using std::string;
using cryo::trees;

TEST_CASE("adding stuff"){
    trees meister = trees<int>(4);
    trees<int>::tree t1 = meister.get();
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    trees<int>::tree t2 = t1.add(7);
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    CHECK(t2.size()==2);
    CHECK(t2[0]==4);
    CHECK(t2[1]==7);
    trees<int>::tree t3 = t1.add(5);
    trees<int>::tree t4 = t3.insert(0, 42);
    CHECK(t3[1]==5);
    CHECK(t4[0]==42);
}

TEST_CASE("add-multiple"){
    trees meister = trees<int>(42);
    trees<int>::tree tree1 = meister.get();
    trees<int>::tree tree2 = tree1.add({1,2,3,4,5,6,7,8,9});
    CHECK(tree2.size()==10);
    CHECK(tree2[0]==42);
    CHECK(tree2[4]==4);
}

TEST_CASE("strings") { //strings machen stress
    trees meister = trees<string>("hallo");
    trees<string>::tree tree1 = meister.get();
    trees<string>::tree tree2 = tree1.add("hi");
    CHECK(tree2[1]=="hi");
}

TEST_CASE("structs"){
    struct point {
        int x, y, z;
        double d;
    };
    point p1 = {0,2,5,5.3};
    point p2 = {6,1,5,-9.3};
    point p3 = {0,2,5,54.3};
    trees meister = trees<point>();
    trees<point>::tree tree1 = meister.get();
    trees<point>::tree tree2 = tree1.add(p1);
    trees<point>::tree tree3 = tree2.add(p2);
    trees<point>::tree tree4 = tree3.add(p3);
    CHECK(tree4.size()==3);
}

TEST_CASE("many elements") {
    auto meister = trees<int>();
    auto t1 = meister.get();
    std::vector<trees<int>::tree> ts;
    ts.push_back(t1.add(0));
    int N = 145;
    for (int i=1;i<N;i++){
        ts.push_back(ts[i-1].add(i));
    }

    for (int i=0;i<N;i++){
        CHECK(ts[i].size()==i+1);
        CHECK(ts[i][i]==i);
    }
}

TEST_CASE("many many elements") {
    auto meister = trees<int>();
    auto t1 = meister.get();
    std::vector<trees<int>::tree> ts;
    ts.push_back(t1.add(0));
    int N = 252622;
    for (int i=1;i<N;i++){
        ts.push_back(ts[i-1].add(i));
    }
    for (int i=0;i<N;i++){
        CHECK(ts[i].size()==i+1);
        CHECK(ts[i][i]==i);
    }
}

TEST_CASE("small B") {
    auto meister = trees<int, 2>(4);
    auto t1 = meister.get();
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    auto t2 = t1.add(7);
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    CHECK(t2.size()==2);
    CHECK(t2[0]==4);
    CHECK(t2[1]==7);
    auto t3 = t1.add(5);
    auto t4 = t3.insert(0, 42);
    CHECK(t3[1]==5);
    CHECK(t4[0]==42);
}

TEST_CASE("iterator") {
    auto meister = trees<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    auto it = t1.begin();
    while (it!=t1.end()) {
        cout<<*it<<" ";
        it++;
    }
    cout<<endl;
    for (int i : t1)
        cout<<i<<" ";
    cout<<endl;
}

TEST_CASE("2 iterators") {
    auto m1 = trees<int>({1,2,3,4,5});
    auto m2 = trees<int>({6,7,8,9});
    auto t1 = m1.get(), t2 = m2.get();
    auto i1 = t1.begin(), i2 = t2.begin();
    while (i1!=t1.end()){
        CHECK(i1!=i2);
        i1++, i2++;
    }
}

TEST_CASE("iterator operators") {
    auto m1 = trees<int>({1, 6, 25, 15, -6, 23, 0, 111});
    auto t1 = m1.get();
    auto i1 = t1.begin();
    CHECK(*i1 == 1);
    CHECK(*(i1+3)==15);
    i1+=4;
    CHECK(*i1==-6);
    CHECK(*(i1-2)==25);
    i1--;
    CHECK(*i1==15);
    auto i2 = t1.begin()+3;
    CHECK(i1==i2);
}

TEST_CASE("reverse iterator") {
    auto meister = trees<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    auto it = t1.rbegin();
    while (it!=t1.rend()) {
        cout<<*it<<" ";
        it++;
    }
    cout<<endl;
    for (int i : t1.reverse())
        cout<<i<<" ";
    cout<<endl;
}

TEST_CASE("long string") {
    auto meister = trees<string>();
    auto t1 = meister.get();
    auto t2 = t1.add("looooooooooooooooooooooooooooong string");
    CHECK(t2.size()==1);
    CHECK(t2[0]=="looooooooooooooooooooooooooooong string");
}

TEST_CASE("std::copy") {
    auto meister = trees<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2; //cant copy to different cryo tree due to immutability
    std::copy(t1.begin(),t1.end(),std::back_inserter(t2));
    for (int i=0;i<t1.size();i++)
        CHECK(t1[i]==t2[i]);
}

TEST_CASE("std::copy_if") {
    auto meister = trees<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2;
    std::copy_if(t1.begin(),t1.end(), std::back_inserter(t2), [](int x) {return x%2==0;});
    for (int i : t2) CHECK(i%2==0);
}


TEST_CASE("std::transform (unary)") {
    auto meister = trees<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2(t1.size());
    std::transform(t1.begin(),t1.end(),t2.begin(), [](int x) {return x+1;});
    for (int i=0;i<t1.size();i++)
        CHECK(t2[i]==t1[i]+1);
}

TEST_CASE("std::transform (binary)") {
    auto meister1 = trees<int>({6,3,8,35,1,9});
    auto t1 = meister1.get();
    auto meister2 = trees<int>({9, -3, 52, 11, 45, 0});
    auto t2 = meister2.get();
    std::vector<int> t3(t1.size());
    std::transform(t1.begin(),t1.end(),t2.begin(), t3.begin(), [](int a, int b) {return a+b;});
    for (int i=0;i<t1.size();i++)
        CHECK(t3[i]==t1[i]+t2[i]);
}

TEST_CASE("set_union") {
    auto m1 = trees<int>({1,2,3,5});
    auto t1 = m1.get();
    auto m2 = trees<int>({-1,0,3,4,9});
    auto t2 = m2.get();
    std::set<int> check({-1,0,1,2,3,4,5,9});
    std::vector<int> t3;
    std::set_union(t1.begin(),t1.end(),t2.begin(),t2.end(),std::back_inserter(t3));
    for (int i : t3) {
        CHECK(check.count(i));
        check.erase(i);
    }
}
