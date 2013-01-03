//
//  world.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 31.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

#include <algorithm>
#include <iostream>
#include <ctime>

#include <boost/random.hpp>
#include <boost/bind.hpp>

using namespace economy;

static boost::mt19937          rng;

struct compare_items_rev
{
    bool operator()(const item &i1, const item &i2)
    {
        return i1.tier > i2.tier;
    }
};

world::world() :
_market( _accounts ),
_step( 0 ),
_cycle( 0 )
{
    _players = new player[MAX_PLAYERS];
    rng.seed( (unsigned int) std::time( 0 ) );
}

world::~world()
{
    delete [] _players;
    _players = NULL;
}

void world::generate_items(player &p)
{
    boost::uniform_int<>    it_gen_count_dist(NUM_ITEMS_PER_STEP_MIN,NUM_ITEMS_PER_STEP_MAX);
    
    const int num_items_to_generate = it_gen_count_dist( rng );
    
    for( int i = 0; i < num_items_to_generate; i++ )
    {
        p.stash.push_back( _generator.get_new_item( p ) );
    }
}

void world::use_best_items(player &p)
{
    auto iter = p.stash.begin();
    while( iter != p.stash.end() )
    {
        item it = *iter;
        if( p.used_items[it.slot].tier < it.tier )
        {
            *iter = p.used_items[it.slot];
            p.used_items[it.slot] = it;
            p.last_upgrade = _cycle;
        }
        iter++;
    }
    p.stash.erase( std::remove_if( p.stash.begin(), p.stash.end(), boost::bind( &item::is_empty, _1 ) ), p.stash.end() );
}

void world::destroy_bad_items(int player_id)
{
    player &p=_players[player_id];
    std::sort( p.stash.begin(), p.stash.end(), compare_items_rev() );
    if( p.stash.size() <= MAX_ITEMS_TO_ENLIST )
    {
        return;
    }
    _accounts.add_to_account(player_id, ( p.stash.size() - MAX_ITEMS_TO_ENLIST ) * VENDOR_PRICE );
    p.stash.erase( p.stash.begin() + MAX_ITEMS_TO_ENLIST, p.stash.end() );
}

void world::enlist_stash_items(int player_id)
{
    player &p=_players[player_id];
    auto iter = p.stash.begin();
    while( iter != p.stash.end() )
    {
        const item &it = *iter;
        _market.enlist_item( _step, it, player_id, _market.get_suggested_min_price( it.slot, it.tier ) );
        iter++;
    }
    p.stash.clear();
}

void world::find_upgrades(int player_id)
{
    player &p=_players[player_id];
    bool processed_slots[NUM_ITEM_SLOTS];
    
    for( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        processed_slots[i] = false;
    }
    
    for(int i = 0; i < NUM_ITEM_SLOTS ; i++ )
    {
        int worst_item_index = -1;
        for( int j = 0; j < NUM_ITEM_SLOTS; j++ )
        {
            if( processed_slots[j] )
            {
                continue;
            }
            if( ( worst_item_index == -1 ) ||
               ( p.used_items[worst_item_index].tier > p.used_items[j].tier) )
            {
                worst_item_index = j;
            }
        }
        if(worst_item_index != -1)
        {
            if( p.used_items[worst_item_index].tier == NUM_ITEM_TIERS - 1 )
            {
                //player is maxed out
                return;
            }
            currency_t account = _accounts.get_account( player_id );
            offer o;
            if( _market.find_offer(worst_item_index, p.used_items[worst_item_index].tier + 1, account, o) )
            {
                _market.bid(o, player_id, o.minimum_bid() );
            }
            processed_slots[worst_item_index] = true;
        }
    }
}

void world::process_player(int player_id)
{
    player &p=_players[player_id];
    generate_items( p );
    use_best_items( p );
    destroy_bad_items( player_id );
    enlist_stash_items( player_id );
    find_upgrades( player_id );
}

void world::finalize_transactions(const std::vector<offer> &transactions)
{
    auto iter = transactions.begin();
    while( iter != transactions.end() )
    {
        const offer &o = *iter;
        if(o.has_buyer)
        {
            _players[o.buyer_id].stash.push_back(o.it);
            currency_t tax = TRANSACTION_TAX * o.current_bid /  100;
            _accounts.add_to_account( o.seller_id, o.current_bid - tax );
            
            _cycle_stats.successful_transactions[o.it.tier]++;
            _cycle_stats.transaction_volume[o.it.tier] += o.current_bid;
            _cycle_stats.tax[o.it.tier] += tax;
        }
        else
        {
            _players[o.seller_id].stash.push_back(o.it);
            
            _cycle_stats.failed_transactions[o.it.tier]++;
        }
        iter++;
    }
}

void world::process_all()
{
    for( int i = 0; i < MAX_PLAYERS ; i++ )
    {
        _step++;
        process_player( i );
        if( _step % TRANSACTION_FREQUENCY == 0)
        {
            std::vector<offer> transactions;
            _market.remove_old_transactions( _step, transactions );
            finalize_transactions( transactions );
        }
    }
}

int world::get_cycle_upgrade_count()
{
    int count = 0;
    for( int i = 0; i < MAX_PLAYERS ; i++ )
    {
        if( _players[i].last_upgrade == _cycle )
        {
            count++;
        }
    }
    return count;
}

double world::get_avg_item_tier()
{
    double sum = 0.0f;
    for( int i = 0; i < MAX_PLAYERS ; i++ )
    {
        sum += _players[i].get_average_tier();
    }
    return sum / MAX_PLAYERS;
}

void world::begin_reporting()
{
    for(int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"TR_COUNT_T" << i <<"\";";
    }
    for(int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"TR_VOL_T" << i <<"\";";
    }
    for(int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"AVG_PRICE_T" << i <<"\";";
    }
    std::cout << "\"UPG_COUNT\";\"AVG_TIER\"" << std::endl;
}

void world::report_cycle_results()
{
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _cycle_stats.successful_transactions[i] << ";";
    }
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _cycle_stats.transaction_volume[i] << ";";
    }
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _market.get_avg_tier_price(i) << ";";
    }
    std::cout << get_cycle_upgrade_count() << ";";
    std::cout << get_avg_item_tier() << std::endl;
}

void world::run_simulation()
{
    _step = 0;
    begin_reporting();
    for( _cycle = 0; _cycle < NUM_SIMULATION_CYCLES ; _cycle++ )
    {
        _cycle_stats.clear();
        process_all();
        report_cycle_results();
    }
}

