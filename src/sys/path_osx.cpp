#include "path.hpp"
#include "sys/autocf_osx.hpp"
#include <CoreFoundation/CFBundle.h>
namespace Path {

void init()
{
    std::string resources, bundle;
    {
        CFBundleRef main = CFBundleGetMainBundle();
        AutoCF<CFURLRef> resourcesURL(CFBundleCopyResourcesDirectoryURL(main));
        AutoCF<CFURLRef> bundleURL(CFBundleCopyBundleURL(main));
        char buf[1024]; // FIXME maximum?
        bool r;
        r = CFURLGetFileSystemRepresentation(resourcesURL, true, reinterpret_cast<UInt8 *>(buf), sizeof(buf));
        if (r) resources = std::string(buf);
        r = CFURLGetFileSystemRepresentation(bundleURL, true, reinterpret_cast<UInt8 *>(buf), sizeof(buf));
        if (r) bundle = std::string(buf);
    }
    if (!resources.empty()) {
        resources += "/data";
        globalData.push_back(resources);
    }
}

}
