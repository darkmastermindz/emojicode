//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "../Application.hpp"
#include "../CompilerError.hpp"
#include "../EmojicodeCompiler.hpp"
#include "../Generation/VTIProvider.hpp"
#include "../Types/TypeContext.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace EmojicodeCompiler {

void Function::setLinkingTableIndex(int index) {
    linkingTableIndex_ = index;
}

bool Function::enforcePromises(Function *super, const TypeContext &typeContext, const Type &superSource,
                               std::experimental::optional<TypeContext> protocol) {
    try {
        if (super->final()) {
            throw CompilerError(position(), superSource.toString(typeContext), "’s implementation of ", utf8(name()),
                                " was marked 🔏.");
        }
        if (this->accessLevel() != super->accessLevel()) {
            throw CompilerError(position(), "Access level of ", superSource.toString(typeContext),
                                "’s implementation of, ", utf8(name()), ", doesn‘t match.");
        }

        auto superReturnType = protocol ? super->returnType.resolveOn(*protocol) : super->returnType;
        if (!returnType.resolveOn(typeContext).compatibleTo(superReturnType, typeContext)) {
            auto supername = superReturnType.toString(typeContext);
            auto thisname = returnType.toString(typeContext);
            throw CompilerError(position(), "Return type ", returnType.toString(typeContext), " of ", utf8(name()),
                                " is not compatible to the return type defined in ", superSource.toString(typeContext));
        }
        if (superReturnType.storageType() == StorageType::Box && !protocol) {
            returnType.forceBox();
        }
        if (returnType.resolveOn(typeContext).storageType() != superReturnType.storageType()) {
            if (protocol) {
                return false;
            }
            throw CompilerError(position(), "Return and super return are storage incompatible.");
        }

        if (super->arguments.size() != arguments.size()) {
            throw CompilerError(position(), "Argument count does not match.");
        }
        for (size_t i = 0; i < super->arguments.size(); i++) {
            // More general arguments are OK
            auto superArgumentType = protocol ? super->arguments[i].type.resolveOn(*protocol) :
            super->arguments[i].type;
            if (!superArgumentType.compatibleTo(arguments[i].type.resolveOn(typeContext), typeContext)) {
                auto supertype = superArgumentType.toString(typeContext);
                auto thisname = arguments[i].type.toString(typeContext);
                throw CompilerError(position(), "Type ", thisname, " of argument ", i + 1,
                                    " is not compatible with its ", thisname, " argument type ", supertype, ".");
            }
            if (arguments[i].type.resolveOn(typeContext).storageType() != superArgumentType.storageType()) {
                if (protocol) {
                    return false;
                }
                throw CompilerError(position(), "Argument ", i + 1, " and super argument are storage incompatible.");
            }
        }
    }
    catch (CompilerError &ce) {
        package_->app()->error(ce);
    }
    return true;
}

void Function::deprecatedWarning(const SourcePosition &p) const {
    if (deprecated()) {
        if (!documentation().empty()) {
            package_->app()->warn(p, utf8(name()), " is deprecated. Please refer to the "\
                                  "documentation for further information: ", utf8(documentation()));
        }
        else {
            package_->app()->warn(p, utf8(name()), " is deprecated.");
        }
    }
}

void Function::assignVti() {
    if (!assigned()) {
        setVti(vtiProvider_->next());
        for (Function *function : overriders_) {
            function->assignVti();
        }
    }
}

void Function::setUsed(bool enqueue) {
    if (!used_) {
        used_ = true;
        if (vtiProvider_ != nullptr) {
            vtiProvider_->used();
        }
        if (enqueue) {
            package_->app()->compilationQueue.emplace(this);
        }
        for (Function *function : overriders_) {
            function->setUsed();
        }
    }
}

int Function::vtiForUse() {
    assignVti();
    setUsed();
    return vti_;
}

int Function::getVti() const {
    if (!assigned()) {
        throw std::logic_error("Getting VTI from unassinged function.");
    }
    return vti_;
}

void Function::setVti(int vti) {
    if (assigned()) {
        throw std::logic_error("You cannot reassign the VTI.");
    }
    vti_ = vti;
}

bool Function::assigned() const {
    return vti_ >= 0;
}

void Function::setVtiProvider(VTIProvider *provider) {
    if (vtiProvider_ != nullptr) {
        throw std::logic_error("You cannot reassign the VTI provider.");
    }
    vtiProvider_ = provider;
}

Type Function::type() const {
    Type t = Type::callableIncomplete();
    t.genericArguments_.reserve(arguments.size() + 1);
    t.genericArguments_.push_back(returnType);
    for (auto &argument : arguments) {
        t.genericArguments_.push_back(argument.type);
    }
    return t;
}

}  // namespace EmojicodeCompiler
