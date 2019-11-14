#ifndef CVX_GFX_FONT_HPP
#define CVX_GFX_FONT_HPP

#include <string>
#include <vector>

namespace cvx { namespace gfx {

enum class FontStyle { Normal, Oblique, Italic } ;
enum FontWeight { Normal, Bold } ;

class Font {
public:

    Font() = default ;

    Font(const std::string &family_desc, double pts):  sz_(pts),
        style_(FontStyle::Normal),  weight_(FontWeight::Normal) {
        parse_family_names(family_desc) ;
    }

    Font & setStyle(FontStyle style) { style_ = style  ; return *this ; }
    Font & setWeight(FontWeight weight) { weight_ = weight ; return *this ;}
    Font & setSize(double pts) { sz_ = pts ; return *this ; }
    Font & setFamily(const std::string &family_desc ) { parse_family_names(family_desc) ; return *this ; }

    FontStyle style() const { return style_ ; }
    FontWeight weight() const { return weight_ ; }
    double size() const { return sz_ ; }
    const std::vector<std::string> familyNames() const { return family_names_ ; }

    bool operator ==(const Font &other) const {
        if ( family_names_.size() != other.family_names_.size() ) return false ;
        for( int i=0 ; i<family_names_.size() ; i++ ) {
            if ( family_names_[i] != other.family_names_[i] )
                return false ;
        }
        if ( style_ != other.style_ )
            return false ;
        if ( weight_ != other.weight_ )
            return false ;
        if ( sz_ != other.sz_ )
            return false ;
        return true ;
    }

    bool operator !=(const Font &other) const { return !operator == (other) ; }

private:

    void parse_family_names(const std::string &desc) {
        std::string token ;
        for ( char c: desc ) {
            if ( c == ',' ) {
                family_names_.emplace_back(token) ;
                token.clear() ;
            }
            else token += c ;
        }
        if ( !token.empty() ) family_names_.emplace_back(token) ;
    }

    FontStyle style_ = FontStyle::Normal ;
    FontWeight weight_ = FontWeight::Normal ;
    double sz_ = 12 ;
    std::vector<std::string> family_names_ ;
} ;


} }

#endif
