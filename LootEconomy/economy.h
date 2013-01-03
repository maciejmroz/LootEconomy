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
#include <vector>

namespace economy
{
    typedef long            currency_t;     //long is not always 64-bit, take care when porting
    
    const int NUM_ITEM_TIERS = 10;
    const int NUM_ITEM_SLOTS = 10;
    const int MAX_PLAYERS = 10000;
    
    const int TRANSACTION_FREQUENCY = 1000;  //performance optimization
    const int MAX_TRANSACTION_AGE = 15000;
    const double MIN_BID_STEP = 1.05;
    const currency_t VENDOR_PRICE = 100;
    const currency_t MIN_ENLIST_PRICE = 120;
    const int TRANSACTION_TAX = 15;
    
    const int NUM_ITEMS_PER_STEP_MIN = 25;
    const int NUM_ITEMS_PER_STEP_MAX = 75;
    const int MAX_ITEMS_TO_ENLIST = 10;

    const int NUM_SIMULATION_CYCLES = 365;
    
    struct item
    {
        //tier numbering is 1-based, 0 means 'empty'
        int             tier;
        int             slot;

        item()
        {
            clear();
        }
        bool            is_empty() const;
        void            clear();
    };
    
    struct player : public boost::noncopyable
    {
        typedef std::vector<item>       stash_t;
        
        item            used_items[NUM_ITEM_SLOTS];
        stash_t         stash;
        long            last_upgrade;

        player();
        double           get_average_tier() const;
    };
    
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
            return has_buyer ? MIN_BID_STEP * current_bid : min_price;
        }
    };
    
    class item_generator
    {
        //return promotion probability to next tier expressed in percent
        //based on average tier of items used
        int             get_promotion_probability(const player &p);
    public:
        item_generator();
        
        item            get_new_item(const player &p);
    };
    
    class accounts : public boost::noncopyable
    {
        currency_t      *_accounts;
    public:
        accounts();
        ~accounts();
        
        currency_t      get_account(int player_id);
        void            add_to_account(int player_id, currency_t amount);
        void            remove_from_account(int player_id, currency_t amount);
    };
    
    class market : public boost::noncopyable
    {
        typedef std::vector<offer>          offer_data_t;
        
        accounts        &_accounts;
        long            _current_offer_id;
        offer_data_t    _offers[NUM_ITEM_SLOTS][NUM_ITEM_TIERS];
        currency_t      _last_prices[NUM_ITEM_SLOTS][NUM_ITEM_TIERS];

        //call when only last element is not in sorted order (most typical case)
        void            restore_offer_order(offer_data_t &od);
        void            remove_old_transactions(long step, offer_data_t &od,
                                                offer_data_t &result);
    public:
        market(accounts &acc);
        ~market();
        
        void            enlist_item(long step, const item& it, int seller_id, currency_t min_price);
        bool            find_offer(int slot, int min_tier, currency_t max_price, offer &result);
        bool            bid(const offer &off, int buyer_id, currency_t bid_amount);
        currency_t      get_suggested_min_price(int slot, int tier);
        void            remove_old_transactions(long step, std::vector<offer> &result);
        currency_t      get_avg_tier_price(int tier);
    };
    
    struct cycle_statistics
    {
        int             successful_transactions[NUM_ITEM_TIERS];
        int             failed_transactions[NUM_ITEM_TIERS];
        currency_t      transaction_volume[NUM_ITEM_TIERS];
        currency_t      tax[NUM_ITEM_TIERS];
        
        void clear()
        {
            for( int i = 0; i < NUM_ITEM_TIERS; i++ )
            {
                successful_transactions[i] = 0;
                failed_transactions[i] = 0;
                transaction_volume[i] = 0;
                tax[i] = 0;
            }
        }
    };

    class world : public boost::noncopyable
    {
        player              *_players;
        item_generator      _generator;
        accounts            _accounts;
        market              _market;
        long                _step;
        long                _cycle;
        cycle_statistics    _cycle_stats;
        
        void            generate_items(player &p);
        void            use_best_items(player &p);
        void            destroy_bad_items(int player_id);
        void            enlist_stash_items(int player_id);
        void            find_upgrades(int player_id);
        void            process_player(int player_id);
        void            process_all();
        void            finalize_transactions(const std::vector<offer> &transactions);
        
        int             get_cycle_upgrade_count();
        double          get_avg_item_tier();
        void            begin_reporting();
        void            report_cycle_results();
    public:
        world();
        ~world();
        
        void            run_simulation();
    };
}


#endif
