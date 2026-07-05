#include "Flag.hpp"
#include <print>
#include <string_view>
#include <vector>
using namespace std;

int main(int argc, const char* argv[]){
    flag::CmdParser parser;
    vector<int> x;
    string y;
    parser.register_variable(x, "x", {34,45}, "Just some variable x.");
    parser.register_variable(y, "y", "Hello World"s, "Just some variable y.");
    parser.parse(argc, argv);
    vector<string_view> pos_args = parser.get_pos_args();
    println("x : {}, y : {}", x, y);
    for(auto& arg: pos_args){
        println("Posargs: {}", arg);
    }
    return 0;
}
