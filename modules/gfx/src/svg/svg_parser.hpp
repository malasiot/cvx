#ifndef CVX_GFX_SVG_PARSER_HPP
#define CVX_GFX_SVG_PARSER_HPP

#include <string>
#include <cvx/util/misc/dictionary.hpp>
#include <iostream>

#include "svg_dom.hpp"

namespace cvx {
    namespace gfx {
        class SVGLoadException ;
        class SVGDocument ;
    }
}


namespace svg {


class SVGParser {
public:
    SVGParser(cvx::gfx::SVGDocument &doc): document_(doc) {}

    void parseString(const std::string &xml) ;
    void parseStream(std::istream &strm, size_t buffer_sz = 1024) ;
protected:


    template <typename T>
    std::shared_ptr<T> createNode(const Dictionary &a, bool is_root = false) {
        auto node = std::make_shared<T>() ;
        if ( is_root )
            root_ = dynamic_cast<svg::SVGElement *>(node.get()) ;

        node->setDocument(&document_) ;

        auto ele = std::dynamic_pointer_cast<svg::Element>(node) ;

        if ( !nodes_.empty() ) {
            auto stack_node = nodes_.back() ;
            stack_node->addChild(ele) ;
        }
        nodes_.push_back(ele) ;
        node->parseAttributes(a) ;
        return node ;
    }


    void beginElement(const std::string &name, const Dictionary &attributes) ;
    void endElement() ;
    void characters(const std::string &name) ;

private:

    static void start_element_handler(void *data, const char *element_name, const char **attributes) ;
    static void end_element_handler(void *data, const char *elelemnt_name);
    static void character_data_handler(void *data, const char *character_data, int length);

    void handleCharacterData() ;

    std::string processWhiteSpace(const std::string &);

private:

    std::string text_ ;

    cvx::gfx::SVGDocument &document_ ;
    std::deque<std::shared_ptr<svg::Element>> nodes_ ;
    std::deque<std::string> elements_ ;
    svg::SVGElement *root_ = nullptr;
};


}

#endif
