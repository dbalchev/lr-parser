#include <cstdio>
#include "parser.hpp"

#define DOT_CHAR 'O'

#if 0
t -> i
t -> t * i
s -> t
s -> s + t
#endif 


void print (const parser::default_parser::rule &rule)
{
    putchar(rule.get_left_hand());
    printf(" -> ");
    for (int i = 0; i < rule.size(); ++i)
        putchar(rule[i]);
    putchar('\n');
}

void print(const parser::default_parser::item &item)
{
    putchar(item.get_left_hand());
    printf(" -> ");
    for (int i = 0; i < item.get_dot_position(); ++i)
        putchar(item.get_rule()[i]);
    putchar(DOT_CHAR);
    for (int i = item.get_dot_position(); i < item.get_rule().size(); ++i)
        putchar(item.get_rule()[i]);
    putchar('\n');
}
void print(const parser::default_parser::item_set &item_set)
{
    puts("<item-set>");
    for (auto it = item_set.begin(); it != item_set.end(); ++it)
        print(*it);
    puts("</item-set>");
}
typedef parser::default_parser::symbol symbol;
typedef parser::default_parser::rule rule;

struct token
{
    symbol s;
    token()
    {
    }
    const symbol& get_symbol()const
    {
        return s;
    }
    token(const symbol &s)
    {
        this->s = s;
    }
    template<typename ITERATOR>
        token(const rule &r, ITERATOR begin, ITERATOR end)
        {
            s = r.get_left_hand();
            print(r);
        }
};

struct srule
{
    unsigned int left_hand;
    const char * right_hand;
    int precedence;
    int assoc;
    rule get_rule()
    {
        int sz;
        for (sz = 0; right_hand[sz]; ++sz)
            ;
        return rule(left_hand, right_hand, right_hand + sz, precedence, assoc);
    }
};
int main()
{
    try {
        parser::default_parser p;
        unsigned int symbols[] = {'i', '+', 's', 'e', '*', '$', '(', ')'};
        srule rules[] = {
            {'s', "e$", 0, 0},
            {'e', "e+e", 1, 1},
            {'e', "e*e", 2, 1},
            {'e', "(e)", 3, 0},
            {'e', "i", 2, 0}};
        const char *input = "i+i*(i+i)*i+i";
        p.symbols.assign(symbols, symbols + 9);
        for (int i = 0; i < 5; ++i) {
            p.rules.push_back(rules[i].get_rule());
        }            

#if 0
        parser::default_parser::item_set_list graph = p.generate_states('s');
        for (int i = 0; i < graph.size(); ++i) {
            print(graph[i]);
        }
#endif
        fflush(stdout);
        parser::default_parser::context<token> context(p, 's');
        while(*input) 
        {
            context.feed_symbol(*input++);
        }
        context.feed_symbol(token('$'));

    } catch(std::exception &ex) {
        printf("exception: %s\n", ex.what());
    }
    return 0;

}
