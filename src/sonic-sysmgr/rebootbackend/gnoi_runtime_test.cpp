#include <iostream>
#include <string>

#include <google/protobuf/util/json_util.h>
#include "github.com/openconfig/gnoi/system/system.pb.h"

// Exercise the same generated messages and JSON API used by rebootbackend.
// CI also runs this executable against the libraries extracted from the image.
int main() {
    gnoi::system::RebootRequest request;
    auto parsed = google::protobuf::util::JsonStringToMessage(
        R"({"method":"COLD","message":"source runtime probe","force":true})", &request);
    if (!parsed.ok() || request.method() != gnoi::system::COLD || !request.force()) {
        std::cerr << "gNOI JSON parsing failed\n";
        return 1;
    }
    std::string wire;
    gnoi::system::RebootRequest restored;
    if (!request.SerializeToString(&wire) || !restored.ParseFromString(wire) ||
        restored.method() != request.method() || restored.message() != request.message() ||
        restored.force() != request.force()) {
        std::cerr << "gNOI serialization round trip failed\n";
        return 1;
    }
    std::string json;
    if (!google::protobuf::util::MessageToJsonString(restored, &json).ok() ||
        json.find("source runtime probe") == std::string::npos) {
        std::cerr << "gNOI JSON output failed\n";
        return 1;
    }
    std::cout << "gNOI JSON and serialization passed\n";
    return 0;
}
