#include "utils.h"

#include <fstream>
#include <memory>
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <iterator>

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/type_resolver_util.h>

using namespace std;
using namespace google::protobuf;

namespace dash{

static const string GLOBAL_TYPE_URL_PREFIX = "sonic";

// According to the definition, TypeResolver is thread-safe
static unique_ptr<util::TypeResolver> resolver(
    util::NewTypeResolverForDescriptorPool(
        GLOBAL_TYPE_URL_PREFIX,
        DescriptorPool::generated_pool()));

// Table name: DASH_ROUTE_RULE_TABLE
// =>
// Type url: sonic/dash.route_rule.RouteRule
string TableNameToTypeUrl(const string &table_name) {
    static regex rgx("^([^_]*)_(.+)_TABLE$");
    smatch matches;

    if (!regex_match(table_name, matches, rgx)) {
        throw runtime_error("Invalid table name: " + table_name);
    }

    string type_url;
    type_url.reserve(
        GLOBAL_TYPE_URL_PREFIX.size() + 1 
        + matches[1].length() + 1 
        + 2 * (matches[2].length() + 1));
    // Append prefix sonic/
    type_url.append(GLOBAL_TYPE_URL_PREFIX);
    type_url.append("/");
    // Append namespace e.g. dash
    transform(matches[1].first, matches[1].second, back_inserter(type_url), ::tolower);
    type_url.append(".");
    // Append sub namespace e.g. route_rule
    transform(matches[2].first, matches[2].second, back_inserter(type_url), ::tolower);
    type_url.append(".");
    // Append message name. Convert underscore to camel e.g. RouteRule
    bool to_upper = true;
    for (auto it = matches[2].first; it != matches[2].second; ++it) {
        if (*it == '_') {
            to_upper = true;
        } else {
            if (to_upper) {
                type_url.push_back(::toupper(*it));
                to_upper = false;
            } else {
                type_url.push_back(::tolower(*it));
            }
        }
    }

    return type_url;
}

string PbBinaryToJsonString(const string &table_name, const string &binary) {
    auto url = TableNameToTypeUrl(table_name);
    util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    string json;

    // Parse the message type
    auto status = util::BinaryToJsonString(resolver.get(), url, binary, &json, options);
    if (!status.ok()) {
        throw runtime_error(status.message().as_string());
    }

    return json;
}

string JsonStringToPbBinary(const string &table_name, const string &json) {
    auto url = TableNameToTypeUrl(table_name);
    util::JsonParseOptions options;
    options.case_insensitive_enum_parsing = true;
    options.ignore_unknown_fields = false;

    string binary;
    auto status = util::JsonToBinaryString(resolver.get(), url, json, &binary, options);
    if (!status.ok()) {
        throw runtime_error(status.message().as_string());
    }

    return binary;
}

}
