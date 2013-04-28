//
//  economy.h
//  LootEconomy
//
//  Created by Maciej Mróz on 21.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#ifndef LootEconomy_economy_h
#define LootEconomy_economy_h

#include <boost/utility.hpp>
#include <boost/random.hpp>
#include <vector>
#include <array>

namespace economy
{
    typedef long long            currency_t;
    
    const int NUM_ITEM_TIERS = 10;
    const int NUM_ITEM_SLOTS = 10;
    const int MAX_PLAYERS = 10000;
    
    const int TRANSACTION_FREQUENCY = 1000;     //performance optimization, doing it on every cycle would take forever
    const int MAX_TRANSACTION_AGE = 15000;
    const double MIN_BID_STEP = 1.05;
    const currency_t VENDOR_PRICE = 100;
    const currency_t MIN_ENLIST_PRICE = 120;
    const int TRANSACTION_TAX = 15;
    
    const int NUM_ITEMS_PER_STEP_MIN = 25;
    const int NUM_ITEMS_PER_STEP_MAX = 75;
    const int MAX_ITEMS_TO_ENLIST = 10;

    const int NUM_SIMULATION_CYCLES = 365;
    
    class simulation;

    struct item
    {
        int             tier;   //-1 marks empty item
        int             slot;

        item()
        {
            clear();
        }
        bool            is_empty() const;
        void            clear();
    };

    class player
    {
        void            generate_items(simulation &sim);
        void            use_best_items(simulation &sim);
        void            destroy_bad_items();
        void            enlist_stash_items(int player_id, simulation &sim);
        void            find_upgrades(int player_id, simulation &sim);
    public:
        typedef std::vector<item>       stash_t;
        
        std::array<item,NUM_ITEM_SLOTS> used_items;
        stash_t         stash;
        long            last_upgrade;
        currency_t      account;

        player();
        player(const player &p);
        void operator=(const player &p);

        double          get_average_tier() const;
        void            process_simulation_step(simulation &sim, int player_id);
    };

    class item_generator
    {
        //return promotion probability to next tier expressed in percent
        //based on average tier of items used
        int             get_promotion_probability(const player &p);
    public:
        item_generator();
        
        item            get_new_item(const player &p, boost::mt19937 &rng);
    };

    typedef std::vector<player>         players_vec_t;
    
    struct offer
    {
        int             seller_id;
        item            it;
        currency_t      min_price;
        
        bool            has_buyer;
        int             buyer_id;
        currency_t      current_bid;
        long            uid;
        long            create_step;
        
        currency_t price() const
        {
            return has_buyer ? current_bid : min_price;
        }
        
        currency_t minimum_bid() const
        {
            return has_buyer ? (currency_t)(MIN_BID_STEP * current_bid) : min_price;
        }
    };
    
    struct offer_not_found_exception : std::exception {};
    struct bid_amount_too_low_exception : std::exception {};
    struct insufficient_funds_exception : std::exception {};
    
    class market : public boost::noncopyable
    {
        typedef std::vector<offer>          offer_data_t;
        
        players_vec_t   &_players;
        long            _current_offer_id;
        offer_data_t    _offers[NUM_ITEM_SLOTS][NUM_ITEM_TIERS];
        currency_t      _last_prices[NUM_ITEM_SLOTS][NUM_ITEM_TIERS];

        void            validate_bid(const offer &o, int buyer_id, currency_t bid_amount);
        offer&          find_offer(const offer &off);
        //call when only last element is not in sorted order (most typical case)
        void            restore_offer_order(offer_data_t &od);
        void            enlist_item(long step, const item& it, int seller_id, currency_t min_price);
        currency_t      get_suggested_minimum_price(int slot, int tier);
        void            remove_old_transactions(long step, offer_data_t &od,
                                                offer_data_t &result);
        void            remove_old_transactions(long step, std::vector<offer> &result);
    public:
        market(players_vec_t &players_vec);
        ~market();
        
        void            enlist_item(long step, const item& it, int seller_id);
        bool            find_offer(int slot, int min_tier, currency_t max_price, offer &result);
        void            bid(const offer &off, int buyer_id, currency_t bid_amount);
        void            finalize_old_transactions(simulation &sim);
        currency_t      get_average_tier_price(int tier);
    };
    
    struct tier_cycle_statistics
    {
        int             successful_transactions;
        int             failed_transactions;
        currency_t      transaction_volume;
        currency_t      tax;
        
        void clear()
        {
            successful_transactions = 0;
            failed_transactions = 0;
            transaction_volume = 0;
            tax = 0;
        }
    };

    class simulation : public boost::noncopyable
    {
        long            _step;
        long            _cycle;

        void            process_simulation_cycle();
        
        double          get_average_item_tier();
        void            begin_reporting();
        void            report_cycle_results();
    public:
        typedef std::array<tier_cycle_statistics,NUM_ITEM_TIERS> cycle_stats_t;

        boost::mt19937      rng;
        players_vec_t       players;
        item_generator      generator;
        market              market;
        cycle_stats_t       cycle_stats;

        simulation();
        ~simulation();

        long            get_step() {return _step;}
        long            get_cycle() {return _cycle;}

        void            run();
    };
}


#endif
