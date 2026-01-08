#include <iterator>
#include <stdexcept>
#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include<doctest/doctest.h>
#include<cryo/sets.h>
#include"cryo/vectors.h"
#include<iostream>
using std::cout;
using std::endl;
using std::string;
using cryo::vectors;
using cryo::sets;

TEST_CASE("adding stuff"){
    vectors meister = vectors<int>(4);
    vectors<int>::vector t1 = meister.get();
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    vectors<int>::vector t2 = t1.push_back(7);
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    CHECK(t2.size()==2);
    CHECK(t2[0]==4);
    CHECK(t2[1]==7);
    vectors<int>::vector t3 = t1.push_back(5);
    vectors<int>::vector t4 = t3.insert(0, 42);
    CHECK(t3[1]==5);
    CHECK(t4[0]==42);
}

TEST_CASE("push_back-multiple"){
    vectors meister = vectors<int>(42);
    vectors<int>::vector vector1 = meister.get();
    vectors<int>::vector vector2 = vector1.push_back({1,2,3,4,5,6,7,8,9});
    CHECK(vector2.size()==10);
    CHECK(vector2[0]==42);
    CHECK(vector2[4]==4);
}

TEST_CASE("strings") { //strings machen stress
    vectors meister = vectors<string>("hallo");
    vectors<string>::vector vector1 = meister.get();
    vectors<string>::vector vector2 = vector1.push_back("hi");
    CHECK(vector2[1]=="hi");
}

TEST_CASE("structs"){
    struct point {
        int x, y, z;
        double d;
    };
    point p1 = {0,2,5,5.3};
    point p2 = {6,1,5,-9.3};
    point p3 = {0,2,5,54.3};
    vectors meister = vectors<point>();
    vectors<point>::vector vector1 = meister.get();
    vectors<point>::vector vector2 = vector1.push_back(p1);
    vectors<point>::vector vector3 = vector2.push_back(p2);
    vectors<point>::vector vector4 = vector3.push_back(p3);
    CHECK(vector4.size()==3);
}

TEST_CASE("many elements") {
    auto meister = vectors<int>();
    auto t1 = meister.get();
    std::vector<vectors<int>::vector> ts;
    ts.push_back(t1.push_back(0));
    int N = 145;
    for (int i=1;i<N;i++){
        ts.push_back(ts[i-1].push_back(i));
    }

    for (int i=0;i<N;i++){
        CHECK(ts[i].size()==i+1);
        CHECK(ts[i][i]==i);
    }
}

TEST_CASE("many many elements") {
    auto meister = vectors<int>();
    auto t1 = meister.get();
    std::vector<vectors<int>::vector> ts;
    ts.push_back(t1.push_back(0));
    int N = 252622;
    for (int i=1;i<N;i++){
        ts.push_back(ts[i-1].push_back(i));
    }
    for (int i=0;i<N;i++){
        CHECK(ts[i].size()==i+1);
        CHECK(ts[i][i]==i);
    }
}

TEST_CASE("small B") {
    auto meister = vectors<int, 2>(4);
    auto t1 = meister.get();
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    auto t2 = t1.push_back(7);
    CHECK(t1.size()==1);
    CHECK(t1[0]==4);
    CHECK(t2.size()==2);
    CHECK(t2[0]==4);
    CHECK(t2[1]==7);
    auto t3 = t1.push_back(5);
    auto t4 = t3.insert(0, 42);
    CHECK(t3[1]==5);
    CHECK(t4[0]==42);
}

TEST_CASE("iterator") {
    auto meister = vectors<int>({6,3,8,35,1,9});
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
    auto m1 = vectors<int>({1,2,3,4,5});
    auto m2 = vectors<int>({6,7,8,9});
    auto t1 = m1.get(), t2 = m2.get();
    auto i1 = t1.begin(), i2 = t2.begin();
    while (i1!=t1.end()){
        CHECK(i1!=i2);
        i1++, i2++;
    }
}

TEST_CASE("iterator operators") {
    auto m1 = vectors<int>({1, 6, 25, 15, -6, 23, 0, 111});
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
    auto meister = vectors<int>({6,3,8,35,1,9});
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
    auto meister = vectors<string>();
    auto t1 = meister.get();
    auto t2 = t1.push_back("looooooooooooooooooooooooooooong string");
    CHECK(t2.size()==1);
    CHECK(t2[0]=="looooooooooooooooooooooooooooong string");
}

TEST_CASE("std::copy") {
    auto meister = vectors<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2; //cant copy to different cryo vector due to immutability
    std::copy(t1.begin(),t1.end(),std::back_inserter(t2));
    for (int i=0;i<t1.size();i++)
        CHECK(t1[i]==t2[i]);
}

TEST_CASE("std::copy_if") {
    auto meister = vectors<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2;
    std::copy_if(t1.begin(),t1.end(), std::back_inserter(t2), [](int x) {return x%2==0;});
    for (int i : t2) CHECK(i%2==0);
}


TEST_CASE("std::transform (unary)") {
    auto meister = vectors<int>({6,3,8,35,1,9});
    auto t1 = meister.get();
    std::vector<int> t2(t1.size());
    std::transform(t1.begin(),t1.end(),t2.begin(), [](int x) {return x+1;});
    for (int i=0;i<t1.size();i++)
        CHECK(t2[i]==t1[i]+1);
}

TEST_CASE("std::transform (binary)") {
    auto meister1 = vectors<int>({6,3,8,35,1,9});
    auto t1 = meister1.get();
    auto meister2 = vectors<int>({9, -3, 52, 11, 45, 0});
    auto t2 = meister2.get();
    std::vector<int> t3(t1.size());
    std::transform(t1.begin(),t1.end(),t2.begin(), t3.begin(), [](int a, int b) {return a+b;});
    for (int i=0;i<t1.size();i++)
        CHECK(t3[i]==t1[i]+t2[i]);
}

TEST_CASE("set_union") {
    auto m1 = vectors<int>({1,2,3,5});
    auto t1 = m1.get();
    auto m2 = vectors<int>({-1,0,3,4,9});
    auto t2 = m2.get();
    std::set<int> check({-1,0,1,2,3,4,5,9});
    std::vector<int> t3;
    std::set_union(t1.begin(),t1.end(),t2.begin(),t2.end(),std::back_inserter(t3));
    for (int i : t3) {
        CHECK(check.count(i));
        check.erase(i);
    }
}

TEST_CASE("sets") {
    auto m1 = sets<int>(1);
    auto s1 = m1.get();
    auto s2=s1.insert(2);
    CHECK(s1.contains(1));
    CHECK(s2.contains(1));
    CHECK(s2.contains(2));
    CHECK(!s1.contains(2));
    CHECK(s1.size()==1);
    CHECK(s2.size()==2);
}

TEST_CASE("sets strings") {
    auto m1 = sets<string>("a");
    auto s1 = m1.get();
    auto s2=s1.insert("b");
    CHECK(s1.contains("a"));
    CHECK(s2.contains("a"));
    CHECK(s2.contains("b"));
    CHECK(!s1.contains("b"));
    CHECK(s1.size()==1);
    CHECK(s2.size()==2);
}

TEST_CASE("sets long strings") {
    auto meister = sets<string>();
    auto t1 = meister.get();
    auto t2 = t1.insert("looooooooooooooooooooooooooooong string");
    CHECK(t2.size()==1);
    CHECK(t2.contains("looooooooooooooooooooooooooooong string"));  
}

TEST_CASE("sets adding stuff") {
    auto m1 = sets<int>(1);
    auto s1 = m1.get();
    auto s2=s1.insert(2);
    auto s3 = s2.insert({3,4,5,6,7,8});
    auto m2 = sets<int>({1,2,3,4,5});
    auto t1 = m2.get();
    auto t2 = t1.insert(6);

    CHECK(s1.size()==1);
    CHECK(s1.contains(1));
    CHECK(s2.size()==2);
    CHECK(s2.contains(2));
    CHECK(!s2.contains(3));
    CHECK(s3.size()==8);
    for (int i=3;i<=8;i++) CHECK(s3.contains(i));
    CHECK(t1.size()==5);
    for (int i=1;i<=5;i++) CHECK(t1.contains(i));
    CHECK(t2.size()==6);
    for (int i=1;i<=6;i++) CHECK(s3.contains(i));
    CHECK(!t1.contains(6));
}

TEST_CASE("sets iterators") {
    auto meister = sets<int>({6,3,8,35,1,9});
    std::vector<int> check = {1,3,6,8,9,35};
    auto t1 = meister.get(); 
    auto it = t1.begin();
    CHECK(*it==check[0]);
    for (int i=1;i<=5;i++) {
        it++;
        CHECK(*it==check[i]);
    }
    for (int i=4; i>=0;i--) {
        it--;
        CHECK(*it==check[i]);
    }
}