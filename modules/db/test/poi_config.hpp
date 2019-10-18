#ifndef POICONFIG_HPP
#define POICONFIG_HPP

#include <string>
#include <vector>
#include <map>

#include <cvx/util/misc/json_reader.hpp>

class POIConfig {
public:
    POIConfig();

    struct Rule {
        std::string key_, val_ ;
    };

    struct Category {
        std::string id_ ;
        std::map<std::string, std::string> lnames_ ;
        std::vector<Rule> rules_ ;
    };

    bool parse(const std::string &path) ;

    std::vector<Category> categories_ ;
    std::vector<std::string> languages_ ;

private:
    void parseCategory(cvx::util::JSONReader &) ;
    void parseRule(cvx::util::JSONReader &, Category &cat) ;
};

#endif // POICONFIG_HPP
