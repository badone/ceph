#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <sstream>

int main() {

    unsigned long long v = 0xa00000000004000;
    std::bitset<64> bs(v);


    std::stringstream feature_sstream;
    std::ifstream ifile("../../src/include/ceph_features.h", std::ios::in | std::ios::binary);
    feature_sstream << ifile.rdbuf();
    ifile.close();

    std::string features = feature_sstream.str();
    std::regex reg("DEFINE_CEPH_FEATURE.*\\(\\s?(\\d+),.*\\)");
    std::sregex_iterator it(features.begin(), features.end(), reg);
    std::multimap<int, std::string> feature_map;
    for (auto end = std::sregex_iterator(); it != end; ++it) {
        auto match = *it;
        feature_map.insert(std::make_pair(std::stoi(match[1]), match[0]));
    }

    std::cout << "The following bits (features) are set for input value " <<
        std::hex << "0x" << v << std::dec << std::endl << std::endl;
    for (uint x = 0; x < bs.size(); x++) {
        if (bs.test(x)) {
            auto results = feature_map.equal_range(x);
            for (auto& r = results.first; r != results.second; ++r)
                std::cout << r->first << " " << r->second << std::endl;
        }
    }

    return 0;
}
