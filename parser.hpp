#ifndef PARSER_HPP
#define PARSER_HPP
// vim: set cino=; set sw=4; set ts=4
#include <stdexcept>
#include <algorithm>
#include <intrin.h>
#ifndef DONT_DEFINE_PARSER_DEFAULT

#include <vector>
#include <set>

#endif

namespace parser {
    enum ASSOCIATIVITY {
        NOASSOC,
        LEFT_ASSOC,
        RIGHT_ASSOC
    };
    enum ACTION_TYPE {
        REDUCE,
        SHIFT,
        INVALID_ACTION,
    };
#ifndef DONT_DEFINE_PARSER_DEFAULT

    struct default_parser_params {
        typedef unsigned int uint;
        typedef uint symbol;
        template<typename T>
            struct types {
                typedef std::vector<T> vector;
                typedef std::set<T> set;
                struct indexed_stack : vector
                {
                    void pop(uint n = 1)
                    {
                        resize(size() - n);
                    }
                    size_t size() const
                    {
                        return vector::size();
                    }
                };
            };
    };

#endif

    template<typename PARSER_PARAMS>
        struct parser_types : PARSER_PARAMS {

            typedef typename PARSER_PARAMS::uint uint;
            typedef typename PARSER_PARAMS::symbol symbol;
            template <typename T>
                struct types : PARSER_PARAMS::template types<T>{};
            typedef uint ASSOCIATIVITY;
            typedef uint ACTION_TYPE;
            struct rule : PARSER_PARAMS::template types<symbol>::vector {
                typedef typename PARSER_PARAMS::template types<symbol>::vector vector;
                symbol left_hand;
                int precedence;
                ASSOCIATIVITY associativity;
                template<typename SYMBOL_ITERATOR>
                    rule(const symbol& left_hand, SYMBOL_ITERATOR begin, SYMBOL_ITERATOR end, int precedence = 0, ASSOCIATIVITY associativity = NOASSOC)
                    :vector(begin, end), left_hand(left_hand), precedence(precedence), associativity(associativity)
                    {
                    }

                const symbol& get_left_hand() const
                {
                    return left_hand;
                }
                bool operator<(const rule& rh) const
                {
                    if (left_hand == rh.left_hand)
                        return (const vector&)*this < (const vector&) rh;
                    return left_hand < rh.left_hand;
                }
                bool operator==(const rule& rh) const
                {
                    return left_hand == rh.left_hand && (const vector&)*this == (const vector&) rh;
                }
                int get_precedence() const 
                {
                    return precedence;
                }
                ASSOCIATIVITY get_associativity() const
                {
                    return associativity;
                }

            };
            struct item
            {
                uint dot_position;
                rule on_rule;

                item(rule on_rule)
                    :dot_position(0), on_rule(on_rule)
                {}
                const symbol& get_left_hand() const
                {
                    return on_rule.get_left_hand();
                }
                const uint& get_dot_position()const
                {
                    return dot_position;
                }
                const rule& get_rule() const
                {
                    return on_rule;
                }
                const symbol& next_symbol() const
                {
                    if (at_end())
                        throw std::runtime_error("cannot get next symbol if at end");
                    return on_rule[dot_position];
                }
                bool can_progress(const symbol &s) const
                {
                    return dot_position < on_rule.size() && on_rule[dot_position] == s;
                }
                bool at_end() const
                {
                    return dot_position == on_rule.size();
                }
                item next() const
                {
                    if (at_end())
                        throw std::runtime_error("cannot progress after last symbol");
                    return item(dot_position + 1, on_rule);
                }
                bool operator<(const item& rh) const
                {
                    if (dot_position == rh.dot_position)
                        return on_rule < rh.on_rule;
                    return dot_position < rh.dot_position;
                }
                bool operator ==(const item& rh) const
                {
                    return dot_position == rh.dot_position && on_rule == rh.on_rule;
                }
                protected:
                item(uint dot_position, rule on_rule)
                    :dot_position(dot_position), on_rule(on_rule)
                {
                    if (dot_position > on_rule.size())
                        throw std::runtime_error("dot_position must be < on_rule.size()");
                }
            };

            struct action
            {
                ACTION_TYPE type;
                const rule* on_rule;

                action(const action &action)
                    :type(action.type), on_rule(action.on_rule)
                {
                }

                action(ACTION_TYPE type, const rule* on_rule)
                    :type(type), on_rule(on_rule)
                {}
                action(ACTION_TYPE type, const rule& on_rule)
                    :type(type), on_rule(&on_rule)
                {}
                static action shift(const rule& on_rule)
                {
                    return action(SHIFT, on_rule);//used for precedence
                }
                static action reduce(const rule& on_rule)
                {
                    return action(REDUCE, on_rule);
                }
                ACTION_TYPE get_type() const
                {
                    return type;
                }
                const rule& get_rule() const
                {
                    if (type == SHIFT)
                        throw std::runtime_error("shouldn't get rule of a shift action");
                    return *on_rule;
                }
                int get_precedence() const
                {
                    return on_rule->get_precedence();
                }
                ASSOCIATIVITY get_associativity() const
                {
                    return on_rule->get_associativity();
                }

            };

            struct item_set : PARSER_PARAMS::template types<item>::set
            {
                typedef typename PARSER_PARAMS::template types<item>::set set;
                typedef typename set::const_iterator const_iterator;

                const_iterator begin() const
                {
                    return set::begin();
                }
                const_iterator end() const
                {
                    return set::end();
                }

                item_set()
                {}
                template <typename ITERATOR_TYPE>
                    item_set(ITERATOR_TYPE b, ITERATOR_TYPE e)
                    :set(b, e)
                    {}
                template <typename CALLBACK_TYPE>
                    void enumerate_actions(const symbol &lookup_symbol, CALLBACK_TYPE &callback) const
                    {
                        const_iterator item;
                        for (item = begin(); item != end(); ++item) {
                            if (item->at_end())
                                callback(action::reduce(item->get_rule()));
                            else
                                if (item->next_symbol() == lookup_symbol)
                                    callback(action::shift(item->get_rule()));
                        }

                    }
                struct get_action_callback
                {
                    action result;
                    get_action_callback()
                        :result(action(INVALID_ACTION, 0))
                    {}
                    void operator()(const action& new_action)
                    {
                        const char *message = "no error?";
                        if (result.get_type() == INVALID_ACTION) {
                            goto update;
                        }
                        if (result.get_precedence() > new_action.get_precedence())
                            return;
                        if (result.get_precedence() < new_action.get_precedence()) {
                            goto update;
                        }
                        {
                            ASSOCIATIVITY old_assoc = result.get_associativity(),
                                          new_assoc = new_action.get_associativity();
                            if (old_assoc != new_assoc) {
                                message = "conflict: actions with same precedence with different associativity";
                                goto conflict;
                            }
                            if (old_assoc == NOASSOC) {
                                message = "conflict: actions with no associativity require it";
                                goto conflict;
                            }
                            ACTION_TYPE old_action_type = result.get_type(),
                                        new_action_type = new_action.get_type();
                            if (old_action_type == REDUCE && new_action_type == REDUCE) {
                                message = "reduce/reduce conflict";
                                goto conflict;
                            }
                            if (old_assoc == LEFT_ASSOC && new_action_type == REDUCE)
                                goto update;
                            if (old_assoc == RIGHT_ASSOC && new_action_type == SHIFT)
                                goto update;
                            return;
                        }
update:
                        result = new_action;
                        return;
conflict:
                        throw std::runtime_error(message);

                    }
                };
                action get_action(const symbol &lookup_symbol) const
                {
#if 0
                    extern void print(const item_set &item_set);
                    puts("CURRENT STATE");
                    print(*this);
#endif
                    get_action_callback callback;
                    enumerate_actions(lookup_symbol, callback);
                    return callback.result;
                }
            };
            typedef typename PARSER_PARAMS::template types<item_set>::vector item_set_list;
        };

#ifndef DONT_DEFINE_PARSER_DEFAULT

    struct default_parser_impl: parser_types<default_parser_params>
    {
        typedef parser_types<default_parser_params> base;
        typedef std::vector<rule>::const_iterator rule_iterator;
        typedef std::vector<symbol>::const_iterator symbol_iterator;
        template <typename T>
            struct types : base::template types<T>{};

        rule_iterator begin_rules() const
        {
            return rules.begin();
        }
        rule_iterator end_rules() const
        {
            return rules.end();
        }
        symbol_iterator begin_symbols() const
        {
            return symbols.begin();
        }
        symbol_iterator end_symbols() const
        {
            return symbols.end();
        }
        //implementation defined
        std::vector<rule>    rules;
        std::vector<symbol> symbols;

    };
#endif
    template <typename PARSER_IMPL>
        struct parser : PARSER_IMPL
    {
        typedef typename PARSER_IMPL::item_set item_set;
        typedef typename PARSER_IMPL::uint uint;
        typedef typename PARSER_IMPL::symbol symbol;
        typedef typename PARSER_IMPL::item item;
        typedef typename PARSER_IMPL::rule rule;
        typedef typename PARSER_IMPL::rule_iterator rule_iterator;
        typedef typename PARSER_IMPL::symbol_iterator symbol_iterator;   
        typedef typename PARSER_IMPL::item_set_list item_set_list;
        typedef typename PARSER_IMPL::action action;
        template <typename T>
            struct types : PARSER_IMPL::template types<T>{};

        rule_iterator begin_rules() const
        {
            return PARSER_IMPL::begin_rules();
        }
        rule_iterator end_rules() const
        {
            return PARSER_IMPL::end_rules();
        }
        symbol_iterator begin_symbols() const
        {
            return PARSER_IMPL::begin_symbols();
        }
        symbol_iterator end_symbols() const
        {
            return PARSER_IMPL::end_symbols();
        }


        template <typename TOKEN_TYPE> 
            struct context
            {
                const parser* active_parser;
                typename types<item_set>::indexed_stack parse_stack;
                typename types<TOKEN_TYPE>::indexed_stack token_stack;
                context(const parser &active_parser, const symbol& initial_symbol)
                    :active_parser(&active_parser)
                {                                   
                    parse_stack.push_back(active_parser.symbol2state(initial_symbol));
                }

                context(const parser &active_parser, const item_set& initial_state)
                    :active_parser(&active_parser), parse_stack(1, initial_state)
                {
                }
                struct default_action
                {
                    template <typename TOKEN_ITERATOR>
                        TOKEN_TYPE operator()(const rule& rule, TOKEN_ITERATOR begin, TOKEN_ITERATOR end)
                        {
                            return TOKEN_TYPE(rule, begin, end);
                        }
                };
                void feed_symbol(const TOKEN_TYPE& lookup_token)
                {
                    //                   printf("lookup = %c\n", lookup_token.get_symbol());
                    feed_symbol(lookup_token, default_action());
                }
                template <typename REDUCE_CALLBACK>
                    void feed_symbol(const TOKEN_TYPE& lookup_token, REDUCE_CALLBACK callback)
                    {
                        for(;;) {
                            const item_set& top = parse_stack.back();
                            const symbol& lookup = lookup_token.get_symbol();
                            action needed_action = top.get_action(lookup);
                            switch(needed_action.get_type()) {
                                case INVALID_ACTION:
                                    throw std::runtime_error("syntax error");
                                case SHIFT:
                                    parse_stack.push_back(active_parser->next(top, lookup));
                                    token_stack.push_back(lookup_token);
                                    return;
                                case REDUCE:
                                    uint num_tokens = needed_action.get_rule().size();
                                    TOKEN_TYPE new_token = 
                                        callback(needed_action.get_rule(), token_stack.end() - num_tokens, token_stack.end());
                                    parse_stack.pop(num_tokens);
                                    token_stack.pop(num_tokens);
                                    const item_set &current_top = parse_stack.back();
                                    const item_set new_top = active_parser->next(current_top, needed_action.get_rule().get_left_hand());
                                    parse_stack.push_back(new_top);
                                    token_stack.push_back(new_token);
                                    break;
                            }
                        }

                    }
            };
        item_set next(const item_set& old_set, const symbol& symbol) const
        {
            item_set pre_rez;
            for(typename item_set::const_iterator it = old_set.begin();
                    it != old_set.end();
                    ++it)
                if (it->can_progress(symbol))
                    pre_rez.insert(it->next());
            return closure(pre_rez);
        }
        item_set closure(const item_set& set) const
        {
            item_set rez(set.begin(), set.end());

            for (typename item_set::const_iterator original_set_element = set.begin();
                    original_set_element != set.end();
                    ++original_set_element) {
                if (original_set_element->at_end())
                    continue;
                symbol required_symbol = original_set_element->next_symbol();
                for (rule_iterator rule_element = begin_rules();
                        rule_element != end_rules();
                        ++rule_element) {
                    if (rule_element->get_left_hand() == required_symbol)
                        rez.insert(item(*rule_element));
                }
            }
            return rez;
        }
        item_set symbol2state(const symbol &start_symbol)const
        {
            item_set initial_state;
            for (rule_iterator rule_element = begin_rules();
                    rule_element != end_rules();
                    ++rule_element) {
                if (rule_element->get_left_hand() == start_symbol)
                    initial_state.insert(item(*rule_element));
            }
            return closure(initial_state);
        }
        item_set_list generate_states(const symbol& start_symbol)
        {
            item_set_list graph;
            {
                item_set initial_state = symbol2state(start_symbol);
                if (initial_state.empty())
                    throw std::runtime_error("initial state is empty item set");
                graph.push_back(closure(initial_state));
            }
            for (uint i = 0; i < graph.size(); ++i) {
                for (symbol_iterator next_symbol = begin_symbols(); 
                        next_symbol != end_symbols();
                        ++next_symbol) {
                    item_set new_state;
                    for (typename item_set::iterator item_to_try = graph[i].begin();
                            item_to_try != graph[i].end();
                            ++item_to_try) {
                        if (item_to_try->can_progress(*next_symbol))
                            new_state.insert(item_to_try->next());
                    }
                    new_state = closure(new_state);
                    if (std::find(graph.begin(), graph.end(), new_state) == graph.end())
                        graph.push_back(new_state);
                }
            }
            return graph;
        }
    };
    typedef parser<default_parser_impl> default_parser;
}

#endif
