#include <boost/regex.hpp>

int main(int argc, char **argv)
{
    boost::regex reg(R"(mon\..*\.asok)");
    boost::regex_search("mon.a.asok", reg);

    return 0;
}
