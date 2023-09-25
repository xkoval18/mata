/* nfa-intersection.cc -- Intersection of NFAs
 *
 * This file is a part of libmata.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// MATA headers
#include "mata/nfa/nfa.hh"
#include "mata/nfa/algorithms.hh"
#include <cassert>


using namespace mata::nfa;

namespace {

using pair_to_state_t = std::vector<std::vector<State>>;
using product_map_t = std::unordered_map<std::pair<State,State>,State>;

/**
 * Add transition to the product.
 * @param[out] product Created product automaton.
 * @param[out] product_map Created product map.
 * @param[in] pair_to_process Currently processed pair of original states.
 * @param[in] new_product_symbol_post State transitions to add to the product.
 */
void add_product_symbol_post(Nfa& product, pair_to_state_t & pair_to_state,
                             const State lhs, const State rhs, SymbolPost& new_product_symbol_post) {
        if (new_product_symbol_post.empty()) { return; }

        StatePost &product_state_post{product.delta.mutable_state_post(pair_to_state[lhs][rhs])};
        if (product_state_post.empty() || new_product_symbol_post.symbol > product_state_post.back().symbol) {
            product_state_post.push_back(std::move(new_product_symbol_post));
        }
        else {
            //this case happens when inserting epsilon transitions
            auto symbol_post_it = product_state_post.find(new_product_symbol_post.symbol);
            if (symbol_post_it == product_state_post.end()) {
                product_state_post.insert(std::move(new_product_symbol_post));
            }
            else {
                symbol_post_it->insert(new_product_symbol_post.targets);
            }
        }
    }

/**
 * Create product state and its transitions.
 * @param[out] product Created product automaton.
 * @param[out] product_map Created product map.
 * @param[out] pairs_to_process Set of product states to process
 * @param[in] lhs_target Target state in NFA @c lhs.
 * @param[in] rhs_target Target state in NFA @c rhs.
 * @param[out] product_symbol_post Transitions of the product state.
 */
void create_product_state_and_move(
        Nfa& product_nfa,
        pair_to_state_t & pair_to_state,
        product_map_t & product_map,
        const Nfa& lhs_nfa,
        const Nfa& rhs_nfa,
        std::deque<State>& pairs_to_process,
        const State lhs_target,
        const State rhs_target,
        SymbolPost& product_symbol_post
) {
    State intersect_target;
    if (pair_to_state[lhs_target][rhs_target] == Limits::max_state) {
        //if (pair_to_state[intersect_state_pair_to.first].find(intersect_state_pair_to.second) == pair_to_state[intersect_state_pair_to.first].end()) {
        //if (product_map.find(intersect_state_pair_to) == product_map.end()) {
        intersect_target = product_nfa.add_state();
        //product_map[intersect_state_pair_to] = intersect_state_to;
        pair_to_state[lhs_target][rhs_target] = intersect_target;
        //if (prod_map != nullptr) //this is here to appease tests
        //    (*prod_map)[std::pair(lhs_target,rhs_target)] = intersect_target;

        product_map[std::pair(lhs_target,rhs_target)] = intersect_target;

        pairs_to_process.push_back(lhs_target);
        pairs_to_process.push_back(rhs_target);

        if (lhs_nfa.final[lhs_target] && rhs_nfa.final[rhs_target]) {
            product_nfa.final.insert(intersect_target);
        }
    } else {
        //intersect_state_to = product_map[intersect_state_pair_to];
        intersect_target = pair_to_state[lhs_target][rhs_target];
    }
    //TODO: would push_back and sort at the end be faster?
    product_symbol_post.insert(intersect_target);
}

} // Anonymous namespace.

namespace mata::nfa {

Nfa intersection(const Nfa& lhs, const Nfa& rhs,
                 product_map_t  *prod_map) {
    const Symbol first_epsilon(EPSILON);
    return algorithms::intersection_eps(lhs, rhs, first_epsilon, prod_map);
}

Nfa mata::nfa::algorithms::intersection_eps(
        const Nfa& lhs, const Nfa& rhs, const Symbol first_epsilon,
        product_map_t *prod_map) {
    Nfa product{}; // Product of the intersection.
    // Product map for the generated intersection mapping original state pairs to new product states.
    product_map_t  product_map{};
    std::deque<State> pairs_to_process{}; // Set of state pairs of original states to process.

    pair_to_state_t pair_to_state(lhs.num_of_states(), std::vector<State>(rhs.num_of_states(), Limits::max_state));

    // Initialize pairs to process with initial state pairs.
    for (const State lhs_initial_state : lhs.initial) {
        for (const State rhs_initial_state : rhs.initial) {
            // Update product with initial state pairs.
            const State product_initial_state = product.add_state();

            product_map[std::pair<State,State>(lhs_initial_state, rhs_initial_state)] = product_initial_state;

            pair_to_state[lhs_initial_state][rhs_initial_state] = product_initial_state;
            pairs_to_process.push_back(lhs_initial_state);
            pairs_to_process.push_back(rhs_initial_state);

            product.initial.insert(product_initial_state);

            if (lhs.final[lhs_initial_state] && rhs.final[rhs_initial_state]) {
                product.final.insert(product_initial_state);
            }
        }
    }

    while (!pairs_to_process.empty()) {
        State rhs_source = pairs_to_process.back();;
        pairs_to_process.pop_back();
        State lhs_source = pairs_to_process.back();;
        pairs_to_process.pop_back();
        // Compute classic product for current state pair.

        mata::utils::SynchronizedUniversalIterator<mata::utils::OrdVector<SymbolPost>::const_iterator> sync_iterator(2);
        mata::utils::push_back(sync_iterator, lhs.delta[lhs_source]);
        mata::utils::push_back(sync_iterator, rhs.delta[rhs_source]);

        while (sync_iterator.advance()) {
            std::vector<StatePost::const_iterator> same_symbol_posts = sync_iterator.get_current();
            assert(same_symbol_posts.size() == 2); // One move per state in the pair.

            // Compute product for state transitions with same symbols.
            // Find all transitions that have the same symbol for first and the second state in the pair_to_process.
            // Create transition from the pair_to_process to all pairs between states to which first transition goes
            //  and states to which second one goes.
            Symbol symbol = same_symbol_posts[0]->symbol;
            if (symbol < first_epsilon) {
                SymbolPost product_symbol_post{symbol};
                for (const State lhs_target: same_symbol_posts[0]->targets) {
                    for (const State rhs_target: same_symbol_posts[1]->targets) {
                        create_product_state_and_move(
                                product, pair_to_state, product_map, lhs, rhs, pairs_to_process,
                                lhs_target, rhs_target, product_symbol_post
                        );
                    }
                }
                //add_product_symbol_post(product, pair_to_state, lhs_source, rhs_source, product_symbol_post);
                StatePost &product_state_post{product.delta.mutable_state_post(pair_to_state[lhs_source][rhs_source])};
                product_state_post.push_back(std::move(product_symbol_post));
            }
            else
                break;
        }

        {
            // Add transitions of the current state pair for an epsilon preserving product.

            // Add epsilon transitions, from lhs e-transitions.
            const StatePost& lhs_state_post{lhs.delta[lhs_source] };
            auto lhs_first_epsilon_it = lhs_state_post.first_epsilon_it(first_epsilon);
            if (lhs_first_epsilon_it != lhs_state_post.end()) {
                for (auto lhs_symbol_post = lhs_first_epsilon_it; lhs_symbol_post < lhs_state_post.end(); lhs_symbol_post++) {
                    SymbolPost prod_symbol_post{lhs_symbol_post->symbol };
                    for (const State lhs_target: lhs_symbol_post->targets) {
                        create_product_state_and_move(product, pair_to_state, product_map, lhs, rhs,
                                                       pairs_to_process,
                                                       lhs_target, rhs_source,
                                                       prod_symbol_post);
                    }
                    add_product_symbol_post(product, pair_to_state, lhs_source, rhs_source, prod_symbol_post);
                }
            }

            // Add epsilon transitions, from rhs e-transitions.
            const StatePost& rhs_state_post{rhs.delta[rhs_source] };
            auto rhs_first_epsilon_it = rhs_state_post.first_epsilon_it(first_epsilon);
            if (rhs_first_epsilon_it != rhs_state_post.end()) {
                for (auto rhs_symbol_post = rhs_first_epsilon_it; rhs_symbol_post < rhs_state_post.end(); rhs_symbol_post++) {
                    SymbolPost prod_symbol_post{rhs_symbol_post->symbol };
                    for (const State rhs_target: rhs_symbol_post->targets) {
                        create_product_state_and_move(product, pair_to_state, product_map, lhs, rhs,
                                                       pairs_to_process,
                                                       lhs_source, rhs_target,
                                                       prod_symbol_post);
                    }
                    add_product_symbol_post(product, pair_to_state, lhs_source, rhs_source, prod_symbol_post);
                }
            }
        }
    }

    if (prod_map != nullptr) { *prod_map = product_map; }
    return product;
} // intersection().

} // namespace mata::nfa.
