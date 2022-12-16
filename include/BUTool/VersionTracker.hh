#ifndef __VERSION_TRACKER_HH__
#define __VERSION_TRACKER_HH__ 1

#include <vector>
#include <string>
#include <stdint.h>
class VersionTracker {
public:
  VersionTracker(){
    sVersion = "Not Initialized";
    repositoryURI = "Not Initialized";
  };
  VersionTracker(std::vector<uint32_t> nVer,
		 std::string sVer,
		 std::string repoURI):
    nVersion(nVer),
    sVersion(sVer),
    repositoryURI(repoURI){
  };
  std::vector<uint32_t> GetNumericVer(){return nVersion;};
  std::string           GetHumanVer()  {return sVersion;};
  std::string           GetRepositoryURI(){return repositoryURI;};
private:
  std::vector<uint32_t> nVersion;
  std::string sVersion;
  std::string repositoryURI;
};
#endif
