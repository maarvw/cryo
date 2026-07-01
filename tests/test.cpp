#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <cryo/setmaps.h>
//#include "cryo/vectors.h"

using std::string;
//using cryo::vectors;
using cryo::setmaps;

// TEST_CASE("adding stuff"){
//     vectors meister = vectors<int>(4);
//     auto t1 = meister.get();
//     CHECK(t1.size()==1);
//     CHECK(t1[0]==4);
//     auto t2 = t1.push_back(7);
//     CHECK(t1.size()==1);
//     CHECK(t1[0]==4);
//     CHECK(t2.size()==2);
//     CHECK(t2[0]==4);
//     CHECK(t2[1]==7);
//     auto t3 = t1.push_back(5);
//     auto t4 = t3.insert(0, 42);
//     CHECK(t3[1]==5);
//     CHECK(t4[0]==42);
// }

// TEST_CASE("push_back-multiple"){
//     vectors meister = vectors<int>(42);
//     auto vector1 = meister.get();
//     auto vector2 = vector1.push_back({1,2,3,4,5,6,7,8,9});
//     CHECK(vector2.size()==10);
//     CHECK(vector2[0]==42);
//     CHECK(vector2[4]==4);
// }

// TEST_CASE("strings") { //strings machen stress
//     vectors meister = vectors<string>("hallo");
//     auto vector1 = meister.get();
//     auto vector2 = vector1.push_back("hi");
//     CHECK(vector2[1]=="hi");
// }

// TEST_CASE("structs"){
//     struct point {
//         int x, y, z;
//         double d;
//     };
//     point p1 = {0,2,5,5.3};
//     point p2 = {6,1,5,-9.3};
//     point p3 = {0,2,5,54.3};
//     vectors meister = vectors<point>();
//     vectors<point>::vector vector1 = meister.get();
//     vectors<point>::vector vector2 = vector1.push_back(p1);
//     vectors<point>::vector vector3 = vector2.push_back(p2);
//     vectors<point>::vector vector4 = vector3.push_back(p3);
//     CHECK(vector4.size()==3);
// }

// TEST_CASE("many elements") {
//     auto meister = vectors<int>();
//     auto t1 = meister.get();
//     std::vector<vectors<int>::vector> ts;
//     ts.push_back(t1.push_back(0));
//     int N = 145;
//     for (int i=1;i<N;i++){
//         ts.push_back(ts[i-1].push_back(i));
//     }

//     for (int i=0;i<N;i++){
//         CHECK(ts[i].size()==i+1);
//         CHECK(ts[i][i]==i);
//     }
// }

// TEST_CASE("many many elements") {
//     auto meister = vectors<int>();
//     auto t1 = meister.get();
//     std::vector<vectors<int>::vector> ts;
//     ts.push_back(t1.push_back(0));
//     int N = 252622;
//     for (int i=1;i<N;i++){
//         ts.push_back(ts[i-1].push_back(i));
//     }
//     for (int i=0;i<N;i++){
//         CHECK(ts[i].size()==i+1);
//         CHECK(ts[i][i]==i);
//     }
// }

// TEST_CASE("small B") {
//     auto meister = vectors<int, 2>(4);
//     auto t1 = meister.get();
//     CHECK(t1.size()==1);
//     CHECK(t1[0]==4);
//     auto t2 = t1.push_back(7);
//     CHECK(t1.size()==1);
//     CHECK(t1[0]==4);
//     CHECK(t2.size()==2);
//     CHECK(t2[0]==4);
//     CHECK(t2[1]==7);
//     auto t3 = t1.push_back(5);
//     auto t4 = t3.insert(0, 42);
//     CHECK(t3[1]==5);
//     CHECK(t4[0]==42);
// }

// TEST_CASE("iterator") {
//     auto meister = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister.get();
//     auto it = t1.begin();
//     while (it!=t1.end()) {
//         cout<<*it<<" ";
//         it++;
//     }
//     cout<<endl;
//     for (int i : t1)
//         cout<<i<<" ";
//     cout<<endl;
// }

// TEST_CASE("2 iterators") {
//     auto m1 = vectors<int>({1,2,3,4,5});
//     auto m2 = vectors<int>({6,7,8,9});
//     auto t1 = m1.get(), t2 = m2.get();
//     auto i1 = t1.begin(), i2 = t2.begin();
//     while (i1!=t1.end()){
//         CHECK(i1!=i2);
//         i1++, i2++;
//     }
// }

// TEST_CASE("iterator operators") {
//     auto m1 = vectors<int>({1, 6, 25, 15, -6, 23, 0, 111});
//     auto t1 = m1.get();
//     auto i1 = t1.begin();
//     CHECK(*i1 == 1);
//     CHECK(*(i1+3)==15);
//     i1+=4;
//     CHECK(*i1==-6);
//     CHECK(*(i1-2)==25);
//     i1--;
//     CHECK(*i1==15);
//     auto i2 = t1.begin()+3;
//     CHECK(i1==i2);
// }

// TEST_CASE("reverse iterator") {
//     auto meister = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister.get();
//     auto it = t1.rbegin();
//     while (it!=t1.rend()) {
//         cout<<*it<<" ";
//         it++;
//     }
//     cout<<endl;
//     for (int i : t1.reverse())
//         cout<<i<<" ";
//     cout<<endl;
// }

// TEST_CASE("long string") {
//     auto meister = vectors<string>();
//     auto t1 = meister.get();
//     auto t2 = t1.push_back("looooooooooooooooooooooooooooong string");
//     CHECK(t2.size()==1);
//     CHECK(t2[0]=="looooooooooooooooooooooooooooong string");
// }

// TEST_CASE("std::copy") {
//     auto meister = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister.get();
//     std::vector<int> t2; //cant copy to different cryo vector due to immutability
//     std::copy(t1.begin(),t1.end(),std::back_inserter(t2));
//     for (int i=0;i<t1.size();i++)
//         CHECK(t1[i]==t2[i]);
// }

// TEST_CASE("std::copy_if") {
//     auto meister = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister.get();
//     std::vector<int> t2;
//     std::copy_if(t1.begin(),t1.end(), std::back_inserter(t2), [](int x) {return x%2==0;});
//     for (int i : t2) CHECK(i%2==0);
// }


// TEST_CASE("std::transform (unary)") {
//     auto meister = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister.get();
//     std::vector<int> t2(t1.size());
//     std::transform(t1.begin(),t1.end(),t2.begin(), [](int x) {return x+1;});
//     for (int i=0;i<t1.size();i++)
//         CHECK(t2[i]==t1[i]+1);
// }

// TEST_CASE("std::transform (binary)") {
//     auto meister1 = vectors<int>({6,3,8,35,1,9});
//     auto t1 = meister1.get();
//     auto meister2 = vectors<int>({9, -3, 52, 11, 45, 0});
//     auto t2 = meister2.get();
//     std::vector<int> t3(t1.size());
//     std::transform(t1.begin(),t1.end(),t2.begin(), t3.begin(), [](int a, int b) {return a+b;});
//     for (int i=0;i<t1.size();i++)
//         CHECK(t3[i]==t1[i]+t2[i]);
// }

// TEST_CASE("set_union") {
//     auto m1 = vectors<int>({1,2,3,5});
//     auto t1 = m1.get();
//     auto m2 = vectors<int>({-1,0,3,4,9});
//     auto t2 = m2.get();
//     std::set<int> check({-1,0,1,2,3,4,5,9});
//     std::vector<int> t3;
//     std::set_union(t1.begin(),t1.end(),t2.begin(),t2.end(),std::back_inserter(t3));
//     for (int i : t3) {
//         CHECK(check.count(i));
//         check.erase(i);
//     }
// }

TEST_CASE("sets") {
    auto m1 = setmaps<int>();
    auto s1 = m1.create();
    s1 = m1.insert(s1, 1);
    CHECK(s1.isa_uniq());
    auto s2 = m1.insert(s1,2);
    CHECK(s2.isa_arr());
    CHECK(s1.contains(1));
    CHECK(s2.contains(1));
    CHECK(s2.contains(2));
    CHECK(!s1.contains(2));
    CHECK(s1.size()==1);
    CHECK(s2.size()==2);
}

TEST_CASE("sets strings") {
    auto m1 = setmaps<string>();
    auto s1 = m1.create("a");
    auto s2=m1.insert(s1,"b");
    CHECK(s1.contains("a"));
    CHECK(s2.contains("a"));
    CHECK(s2.contains("b"));
    CHECK(!s1.contains("b"));
    CHECK(s1.size()==1);
    CHECK(s2.size()==2);
}

TEST_CASE("sets long strings") {
    auto meister = setmaps<string>();
    string l = "looooooooooooooooooooooooooooong string";
    auto t1 = meister.create();
    auto t2 = meister.insert(t1,"looooooooooooooooooooooooooooong string");
    CHECK(t2.size()==1);
    CHECK(t2.contains("looooooooooooooooooooooooooooong string"));
}

TEST_CASE("set node mode") {
    auto ms = setmaps<int>();
    auto m0 = ms.create();
    for (int i = 0; i<20; ++i) m0 = ms.insert(m0, i);
    CHECK(m0.isa_node());
    for (int i = 0; i<20; ++i) CHECK(m0.contains(i));
    CHECK(m0.size() == 20);

}

TEST_CASE("map node mode") {
    auto ms = setmaps<int, int>();
    auto m0 = ms.create();
    for (int i = 0; i<20; ++i) m0 = ms.insert(m0, {i,-1});
    CHECK(m0.isa_node());
    for (int i = 0; i<20; ++i) CHECK(m0.contains(i));
    CHECK(m0.size() == 20);

}

TEST_CASE("map basics") {
    auto ms = setmaps<int, int>();
    auto m0 = ms.create();
    m0 = ms.insert(m0,{1,1});
    auto m1 = ms.insert(m0, {1, -1});
    auto m2 = ms.insert(m1, {2, -2});
    auto m3 = ms.insert(m2, {5,9});
    auto m4 = ms.insert(m3, {1, 42});
    CHECK(m1.contains(1));
    CHECK(m2.contains(1));
    CHECK(m2.contains(2));
    CHECK(m3.contains(1));
    CHECK(m3.contains(2));
    CHECK(m3.contains(5));
    CHECK(m4.contains(1));
    CHECK(m4.contains(2));
    CHECK(m4.contains(5));
    CHECK(m1[1]==-1);
    CHECK(m2[1]==-1);
    CHECK(m2[2]==-2);
    CHECK(m3[1]==-1);
    CHECK(m3[5]==9);
    CHECK(m3[2]==-2);
    CHECK(m4[1]==42);
    CHECK(m1.size()==1);
    CHECK(m2.size()==2);
    CHECK(m3.size()==3);
    CHECK(m4.size()==3);
}

TEST_CASE("map iterator") {
    auto ms = setmaps<int,int>();
    auto m1 = ms.create({0, 0});
    auto m2 = ms.insert(m1,{1,-1});
    auto m3 = ms.insert(m2,{2,-2});
    auto m4 = ms.insert(m3,{3,-3});
    auto m5 = ms.insert(m4,{4,-4});
    auto m6 = ms.insert(m5,{5,-5});
    int i=0;
    CHECK(m6.size()==6);
    for (auto kv : m6) {
        CHECK(m6[i]==-i);
        CHECK(kv.second==-i);
        i++;
    }
}

// TEST_CASE("sets adding 1") {
//     // auto m1 = setmaps<int>();
//     // auto s1 = m1.create();
//     // s1 = m1.insert(s1, 1);
//     // auto s2=m1.insert(s1,2);
//     // auto s3 = *s2.insert({3,4,5,6,7,8});
//     // auto m2 = setmaps<int>({1,2,3,4,5});
//     // auto t1 = m2.create();
//     // auto t2 = *t1.insert(6);

//     // CHECK(s1.size()==1);
//     // CHECK(s1.contains(1));
//     // CHECK(s2.size()==2);
//     // CHECK(s2.contains(2));
//     // CHECK(!s2.contains(3));
//     // CHECK(s3.size()==8);
//     // for (int i=3;i<=8;i++) CHECK(s3.contains(i));
//     // CHECK(t1.size()==5);
//     // for (int i=1;i<=5;i++) CHECK(t1.contains(i));
//     // CHECK(t2.size()==6);
//     // for (int i=1;i<=6;i++) CHECK(s3.contains(i));
//     // CHECK(!t1.contains(6));
// }

// TEST_CASE("set adding 2") {
//     // auto m1 = setmaps<double>({5.1,0.9,-6,4});
//     // auto s1=m1.create();
//     // auto s2=s1->insert({8.2,10.6,-55.2,-5.9,4.4,1.1});
//     // for (double u : {5.1,0.9,-6.0,4.0}) {
//     //     CHECK(s1->contains(u));
//     //     CHECK(s2->contains(u));
//     // }
//     // for (double u : {8.2,10.6,-55.2,-5.9,4.4,1.1}) {
//     //     CHECK(!s1->contains(u));
//     //     CHECK(s2->contains(u));
//     // }
//     // double prev = -999;
//     // auto cur = s2->begin();
//     // while (cur!=s2->end()){
//     //     CHECK(*cur>prev);
//     //     CHECK(s2->contains(*cur));
//     //     prev=*cur;
//     //     cur++;
//     // }
// }

// TEST_CASE("sets iterators") {
//     // auto meister = setmaps<int>({6,3,8,35,1,9});
//     // std::vector<int> check = {1,3,6,8,9,35};
//     // auto t1 = meister.create();
//     // auto it = t1.begin();
//     // CHECK(*it==check[0]);
//     // for (int i=1;i<=5;i++) {
//     //     it++;
//     //     CHECK(*it==check[i]);
//     // }
//     // int i=0;
//     // for (auto i1=t1.begin(); i1!=t1.end(); i1++){
//     //     CHECK(*i1==check[i]);
//     //     i++;
//     // }
//     // CHECK(i==t1.size());

//     // i=0;
//     // for (int v : t1) {
//     //     cout<<i<<" "<<check[i]<<" "<<v<<endl;

//     //     CHECK(v==check[i]);
//     //     i++;
//     // }
//     // CHECK(i==t1.size());
// }

// TEST_CASE("set find") {
//     // auto ms = setmaps<int>(5);
//     // auto m1=ms.create();
//     // auto m2=m1.insert({7,2,6,14,36,85,83,4,1,0});
//     // auto it = m2.find(6);
//     // m2.printtree();
//     // CHECK(*it==6);
//     // ++it;
//     // CHECK(*it==7);
//     // ++it;
//     // CHECK(*it==14);
//     // ++it;
//     // CHECK(*it==36);
// }

// TEST_CASE("set ==") {
//     // auto ms = setmaps<int>(5);
//     // auto m1=ms.create();
//     // auto m2=m1.insert({4,3,2,1});
//     // auto m3 = m1.insert({1,2,3});
//     // CHECK(m2!=m3);
//     // auto m4=m3.insert(4);
//     // CHECK(m2==m4);
// }

// TEST_CASE("set contains_all") {
//     // auto m1 = setmaps<double>({5.1,0.9,-6,4});
//     // auto s1=m1.create();
//     // auto s2=s1->insert({8.2,10.6,-55.2,-5.9,4.4,1.1});
//     // CHECK(s1->contains_all({5.1,0.9,-6,4}));
//     // CHECK(s2->contains_all({5.1,0.9,-6,4}));
//     // CHECK(!s2->contains_all({5.1,45.67,0.9,-6,4}));
//     // CHECK(s2->contains_all({8.2,10.6,-55.2,-5.9,4.4,1.1}));
//     // CHECK(!s1->contains_all({8.2,10.6,-55.2,-5.9,4.4,1.1}));
// }



// TEST_CASE("map contains_all") {
//     // auto m1 = setmaps<double,int>({{5.1,1},{0.9,0},{-6,4},{4,5}});
//     // auto s1=m1.create();
//     // auto s2=s1->insert({{8.2,4},{10.6,4},{-55.2,0},{-5.9,33},{4.4,55},{1.1,-34}});
//     // CHECK(s1->contains_all({5.1,0.9,-6,4}));
//     // CHECK(s2->contains_all({5.1,0.9,-6,4}));
//     // CHECK(s2->contains_all({8.2,10.6,-55.2,-5.9,4.4,1.1}));
//     // CHECK(!s2->contains_all({8.2,10.6,-55.2,-99.9,4.4,1.1}));
//     // CHECK(!s1->contains_all({8.2,10.6,-55.2,-5.9,4.4,1.1}));
// }

// TEST_CASE("map int/string") {
//     // auto ms=setmaps<int,string>({0,"null"});
//     // auto m1=ms.create();
//     // auto m2=m1.insert(6,"sechs");
//     // auto m3=m2.insert({{1,"eins"},{2,"zwei"}});
//     // auto m4=m3.insert(3,"dreidreidreidreidrei");
//     // CHECK(m4[0]=="null");
//     // CHECK(m4[1]=="eins");
//     // CHECK(m4[2]=="zwei");
//     // CHECK(m4[6]=="sechs");
//     // CHECK(m4[3]=="dreidreidreidreidrei");
// }

// TEST_CASE("map string/int") {
//     // auto ms=setmaps<string,int>({"null",0});
//     // auto m1=ms.create();
//     // auto m2=m1.insert("sechs",6);
//     // auto m3=m2.insert({{"eins",1},{"zwei",2}});
//     // auto m4=m3.insert("dreidreidreidreidrei",3);
//     // auto m5 = m4.insert("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 42);
//     // CHECK(m4["null"]==0);
//     // CHECK(m4["eins"]==1);
//     // CHECK(m4["zwei"]==2);
//     // CHECK(m4["sechs"]==6);
//     // CHECK(m4["dreidreidreidreidrei"]==3);
//     // CHECK(m5["aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"]==42);
// }


// TEST_CASE("max depth of sets (insert_prim)") {
//     // auto ms = setmaps<int>(1);
//     // auto m1=ms.create();
//     // auto m2=m1.insert({1,2,3,4,5,6,7,8,9,10});
//     // m2.printtree();
// }

// TEST_CASE("max depth of sets (normal insert)") {
//     // auto ms = setmaps<int>();
//     // auto m1=ms.create();
//     // auto m2=m1.insert(1);
//     // auto m3=m2.insert(2);
//     // auto m4=m3.insert(3);
//     // auto m5=m4.insert(4);
//     // auto m6=m5.insert(5);
//     // auto m7=m6.insert(6);
//     // auto m8=m7.insert(7);
//     // auto m9=m8.insert(8);
//     // auto m10=m9.insert(9);
//     // auto m11=m10.insert(10);
//     // m11.printtree();
// }

// TEST_CASE("map vector") {
//     // auto ms=setmaps<int,std::vector<int>>({1, {1,2,3,4,5}});
//     // auto m1 = ms.create();
//     // auto m2 = m1->insert(1, {1,2,3,4,5});
//     // auto m3 = m2->insert(2,{6,7,8,9});
//     // CHECK((m3)[1][0]==1);
//     // CHECK((m3)[1][1]==2);
//     // CHECK((m3)[1][2]==3);
//     // CHECK((m3)[1][3]==4);
//     // CHECK((m3)[1][4]==5);
//     // CHECK((m3)[2][0]==6);
//     // CHECK((m3)[2][1]==7);
//     // CHECK((m3)[2][2]==8);
//     // CHECK((m3)[2][3]==9);
//     // CHECK(m3->size()==2);
// }

// TEST_CASE("single vec map") {
//     // auto ms=setmaps<int,std::vector<int>>({0,{1,2,3}});
//     // auto m1=ms.create();
// }

// TEST_CASE("single vec set") {
//     // auto ms = setmaps<std::vector<int>>({1,2,3});
//     // auto m1=ms.create();
// }

// TEST_CASE("single vec set 2") {
//     // std::vector<int> v = {1,2,3};
//     // auto ms = setmaps<std::vector<int>>(v);
//     // auto m1=ms.create();
// }

// TEST_CASE("std::greater") {
//     // auto ms = setmaps<int, void, std::greater<int>>({1,2,3});
//     // auto m1 = ms.create();
//     // auto it=m1->begin();
//     // CHECK(*it==3);
//     // ++it;
//     // CHECK(*it==2);
//     // ++it;
//     // CHECK(*it==1);
// }