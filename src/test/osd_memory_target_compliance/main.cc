#include <algorithm>
#include <experimental/filesystem>
#include <iostream>
#include <numeric>
#include <regex>
#include <system_error>

#include <boost/process.hpp>
#include <boost/tokenizer.hpp>

#include "include/rados/librados.hpp"

namespace bp = boost::process;
using namespace std;

unsigned long get_maxallowed()
{
  librados::Rados rados;
  int ret = rados.init("admin");
  if (ret < 0) {
    ret = -ret;
    cerr << "Failed to initialise rados! Error: " << ret << " " << strerror(ret)
         << endl;
    exit(ret);
  }

  rados.conf_parse_env(NULL);
  if (ret < 0) {
    ret = -ret;
    cerr << "Failed to parse environment! Error: " << ret << " "
         << strerror(ret) << endl;
    exit(ret);
  }

  rados.conf_read_file(NULL);
  if (ret < 0) {
    ret = -ret;
    cerr << "Failed to read config file! Error: " << ret << " " << strerror(ret)
         << endl;
    exit(ret);
  }

  ret = rados.connect();
  if (ret < 0) {
    ret = -ret;
    cerr << "Failed to connect to running cluster! Error: " << ret << " "
         << strerror(ret) << endl;
    exit(ret);
  }

  string command =
      R"({"prefix": "config show", "who": "osd.0", "key": "osd_memory_target", "target": ["mgr", ""]})";
  librados::bufferlist inbl, outbl;
  string output;
  ret = rados.mon_command(command, inbl, &outbl, &output);
  if (output.length()) cout << output << endl;
  if (ret < 0) {
    ret = -ret;
    cerr << "Failed to get value of config-key osd_memory_target! Error: "
         << ret << " " << strerror(ret) << endl;
    exit(ret);
  }

  return stoul(string(outbl.c_str(), outbl.length()));
}

int main(int argc, char** argv)
{
  cout << "Querying value of osd_memory_target" << endl;

  auto maxallowed = get_maxallowed();
  cout << "osd_memory_target = " << maxallowed << endl << endl;

  cout << "Processing log files" << endl;
  string target_directory("/var/log/ceph/");
  regex reg(R"(cache_size:\s(\d*)\s)");
  regex logreg(R"(ceph-osd\.\d+\.log)");

  for (const auto& dirent :
       std::experimental::filesystem::directory_iterator(target_directory)) {
    if (!std::regex_match(dirent.path().filename().string(), logreg)) {
      continue;
    }
    string grep_command("grep _trim_shards " + dirent.path().string());
    cout << "Running command \"" << grep_command << "\"" << endl;
    bp::ipstream is;
    error_code ec;
    bp::child grep(grep_command, bp::std_out > is, ec);
    if (ec) {
      cout << "Error grepping logs! Exiting" << endl;
      cout << "Error: " << ec.value() << " " << ec.message() << endl;
      exit(ec.value());
    }

    string line;
    vector<unsigned long> results;
    while (grep.running() && getline(is, line) && !line.empty()) {
      smatch match;
      if (regex_search(line, match, reg)) {
        results.push_back(stoul(match[1].str()));
      }
    }

    if (results.empty()) {
      cout << "Error: No grep results found!" << endl;
      //exit(ENOENT);
      continue;
    }

    auto maxe = *(max_element(results.begin(), results.end()));
    cout << "Results for " << dirent.path().string() << ":" << endl;
    cout << "Max: " << maxe << endl;
    cout << "Min: " << *(min_element(results.begin(), results.end())) << endl;
    auto sum = accumulate(results.begin(), results.end(),
                          static_cast<unsigned long long>(0));
    auto mean = sum / results.size();
    cout << "Mean average: " << mean << endl;
    vector<unsigned long> diff(results.size());
    transform(results.begin(), results.end(), diff.begin(),
              [mean](unsigned long x) { return x - mean; });
    auto sump = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    auto stdev = sqrt(sump / results.size());
    cout << "Standard deviation: " << stdev << endl << endl;

    if (maxe > maxallowed) {
      cout << "Error: Maximum value " << maxe << " exceeds maximum allowed "
           << maxallowed << endl;
      exit(EINVAL);
    }

    grep.wait();
  }

  cout << "Completed successfully" << endl;
  return 0;
}
