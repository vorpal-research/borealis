/*
 * PredicateStateChoice.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "Logging/logger.hpp"
#include "State/PredicateStateChoice.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::head;
using borealis::util::tail;

PredicateStateChoice::PredicateStateChoice(const std::vector<PredicateState::Ptr>& choices) :
        PredicateState(type_id<Self>()),
        choices(choices) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](PredicateState::Ptr s) { return s != nullptr; }));
};

PredicateStateChoice::PredicateStateChoice(std::vector<PredicateState::Ptr>&& choices) :
        PredicateState(type_id<Self>()),
        choices(std::move(choices)) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](PredicateState::Ptr s) { return s != nullptr; }));
};

PredicateState::Ptr PredicateStateChoice::addPredicate(Predicate::Ptr p) const {
    std::vector<PredicateState::Ptr> newChoices;
    newChoices.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(newChoices),
        [&p](PredicateState::Ptr choice) {
            return choice + p;
        }
    );

    return Simplified(new Self(newChoices));
}

logic::Bool PredicateStateChoice::toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx) const {
    TRACE_FUNC;

    using borealis::logic::Bool;

    auto res = z3ef.getFalse();
    std::vector<std::pair<Bool, ExecutionContext>> memories;
    memories.reserve(choices.size());

    for (auto& choice : choices) {
        ExecutionContext choiceCtx(*pctx);

        auto path = choice->filterByTypes({PredicateType::PATH});

        auto z3state = choice->toZ3(z3ef, &choiceCtx);
        auto z3path = path->toZ3(z3ef, &choiceCtx);

        res = res || z3state;
        memories.push_back(std::make_pair(z3path, choiceCtx));
    }

    pctx->switchOn(memories);

    return res;
}

PredicateState::Ptr PredicateStateChoice::addVisited(const llvm::Value* loc) const {
    std::vector<PredicateState::Ptr> newChoices;
    newChoices.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(newChoices),
        [&loc](PredicateState::Ptr choice) {
            return choice << loc;
        }
    );

    return Simplified(new Self(newChoices));
}

bool PredicateStateChoice::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    // FIXME: akhin Just fix this piece of crap
    for (const auto* loc : locs) {
        if (
            std::any_of(choices.begin(), choices.end(),
                [&loc](PredicateState::Ptr choice) {
                    return choice->hasVisited({loc});
                }
            )
        ) continue;
        else return false;
    }
    return true;
}

PredicateState::Ptr PredicateStateChoice::map(Mapper m) const {
    std::vector<PredicateState::Ptr> mapped;
    mapped.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(mapped),
        [&m](PredicateState::Ptr choice) {
            return choice->map(m);
        }
    );

    return Simplified(new Self(mapped));
}

PredicateState::Ptr PredicateStateChoice::filterByTypes(std::initializer_list<PredicateType> types) const {
    std::vector<PredicateState::Ptr> mapped;
    mapped.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(mapped),
        [&types](PredicateState::Ptr choice) {
            return choice->filterByTypes(types);
        }
    );

    return Simplified(new Self(mapped));
}

PredicateState::Ptr PredicateStateChoice::filter(Filterer f) const {
    std::vector<PredicateState::Ptr> mapped;
    mapped.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(mapped),
        [&f](PredicateState::Ptr choice) {
            return choice->filter(f);
        }
    );

    return Simplified(new Self(mapped));
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChoice::splitByTypes(std::initializer_list<PredicateType> types) const {
    std::vector<PredicateState::Ptr> yes;
    std::vector<PredicateState::Ptr> no;
    yes.reserve(choices.size());
    no.reserve(choices.size());

    for (auto& choice: choices) {
        auto split = choice->splitByTypes(types);
        yes.push_back(split.first);
        no.push_back(split.second);
    }

    return std::make_pair(
        Simplified(new Self(yes)),
        Simplified(new Self(no))
    );
}

PredicateState::Ptr PredicateStateChoice::sliceOn(PredicateState::Ptr base) const {
    std::vector<PredicateState::Ptr> slices;
    slices.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(slices),
        [&base](PredicateState::Ptr choice) {
            return choice->sliceOn(base);
        }
    );

    if (std::all_of(slices.begin(), slices.end(),
        [](PredicateState::Ptr slice) { return slice != nullptr; })) {
        return Simplified(new Self(slices));
    }

    return nullptr;
}

PredicateState::Ptr PredicateStateChoice::simplify() const {
    std::vector<PredicateState::Ptr> simplified;
    simplified.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(simplified),
        [](PredicateState::Ptr choice) {
            return choice->simplify();
        }
    );

    simplified.erase(
        std::remove_if(simplified.begin(), simplified.end(),
            [](PredicateState::Ptr choice) { return choice->isEmpty(); })
    );

    if (simplified.size() == 1) {
        return head(simplified);
    } else {
        return PredicateState::Ptr(new Self(simplified));
    }
}

bool PredicateStateChoice::isEmpty() const {
    return std::all_of(choices.begin(), choices.end(),
        [](PredicateState::Ptr choice) {
            return choice->isEmpty();
        }
    );
}

borealis::logging::logstream& PredicateStateChoice::dump(borealis::logging::logstream& s) const {
    using borealis::logging::endl;
    using borealis::logging::il;
    using borealis::logging::ir;

    s << "(BEGIN";
    s << il << endl;
    if (!choices.empty()) {
        s << "<OR>" << head(choices);
        for (const auto& choice : tail(choices)) {
            s << "," << endl << "<OR>" << choice;
        }
    }
    s << ir << endl;
    s << "END)";

    return s;
}

std::string PredicateStateChoice::toString() const {
    using std::endl;

    std::ostringstream s;

    s << "(BEGIN" << endl;
    if (!choices.empty()) {
        s << "<OR>" << head(choices);
        for (const auto& choice : tail(choices)) {
            s << "," << endl << "<OR>" << choice;
        }
    }
    s << endl << "END)";

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"
