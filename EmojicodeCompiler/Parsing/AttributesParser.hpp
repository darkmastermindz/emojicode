//
//  AttributesParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef AttributesParser_hpp
#define AttributesParser_hpp

#include "../Application.hpp"
#include "../Emojis.h"
#include "../Lex/TokenStream.hpp"
#include <array>
#include <map>
#include <string>

namespace EmojicodeCompiler {

struct SourcePosition;
class TokenStream;

enum class Attribute : char32_t {
    Deprecated = E_WARNING_SIGN, Final = E_LOCK_WITH_INK_PEN, Override = E_BLACK_NIB, StaticOnType = E_RABBIT,
    Mutating = E_CRAYON, Required = E_KEY, Export = E_EARTH_GLOBE_EUROPE_AFRICA,
};

template <Attribute ...Attributes>
class AttributeParser {
public:
    AttributeParser& allow(Attribute attr) { found_.find(attr)->second.allowed = true; return *this; }
    bool has(Attribute attr) const { return found_.find(attr)->second.found; }

    void check(const SourcePosition &p, Application *app) const {
        for (auto &pair : found_) {
            if (!pair.second.allowed && pair.second.found) {
                auto name = utf8(std::u32string(1, static_cast<char32_t>(pair.first)));
                app->error(CompilerError(p, "Attribute ", name, " not applicable."));
            }
        }
    }

    AttributeParser& parse(TokenStream *stream) {
        for (auto attr : order_) {
            auto found = stream->consumeTokenIf(static_cast<char32_t>(attr));
            found_.emplace(attr, FoundAttribute(found));
        }
        return *this;
    }
private:
    struct FoundAttribute {
        explicit FoundAttribute(bool found) : found(found) {}
        bool found;
        bool allowed = false;
    };

    constexpr static const std::array<Attribute, sizeof...(Attributes)> order_ = { Attributes... };
    std::map<Attribute, FoundAttribute> found_;
};

template <Attribute ...Attributes>
constexpr const std::array<Attribute, sizeof...(Attributes)> AttributeParser<Attributes...>::order_;

}  // namespace EmojicodeCompiler

#endif /* AttributesParser_hpp */
