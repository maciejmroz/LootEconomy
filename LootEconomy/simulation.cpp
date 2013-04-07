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

using namespace economy;

simulation::simulation() :
_market(_players),
_step(0),
_cycle(0)
{
    _players.resize(MAX_PLAYERS);
    _rng.seed((unsigned int)std::time(0));
}

simulation::~simulation()
{
}

void simulation::finalize_transactions(const std::vector<offer> &transactions)
{
    for( const offer &o : transactions )
    {
        if( o.has_buyer )
        {
            _players[o.buyer_id].stash.push_back(o.it);
            currency_t tax = TRANSACTION_TAX * o.current_bid /  100;
            _players[o.seller_id].account += o.current_bid - tax;
            
            _cycle_stats[o.it.tier].successful_transactions++;
            _cycle_stats[o.it.tier].transaction_volume += o.current_bid;
            _cycle_stats[o.it.tier].tax += tax;
        }
        else
        {
            _players[o.seller_id].stash.push_back(o.it);
            
            _cycle_stats[o.it.tier].failed_transactions++;
        }
    }
}

void simulation::process_simulation_cycle()
{
    for( int i = 0; i < MAX_PLAYERS ; i++ )
    {
        _step++;
        _players[i].process_simulation_step(*this, i);
        if( _step % TRANSACTION_FREQUENCY == 0)
        {
            std::vector<offer> transactions;
            _market.remove_old_transactions(_step, transactions);
            finalize_transactions(transactions);
        }
    }
}

double simulation::get_average_item_tier()
{
    double sum = 0.0;
    for( const player &p : _players)
    {
        sum += p.get_average_tier();
    }
    return sum / _players.size();
}

void simulation::begin_reporting()
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

void simulation::report_cycle_results()
{
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _cycle_stats[i].successful_transactions << ";";
    }
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _cycle_stats[i].transaction_volume << ";";
    }
    for( int i = 0; i < NUM_ITEM_TIERS; i++ )
    {
        std::cout << _market.get_average_tier_price(i) << ";";
    }
    std::cout << std::count_if(_players.begin(),_players.end(),[&] (const player& p) {return p.last_upgrade == _cycle;}) << ";";
    std::cout << get_average_item_tier() << std::endl;
}

void simulation::run()
{
    _step = 0;
    begin_reporting();
    for( _cycle = 0; _cycle < NUM_SIMULATION_CYCLES ; _cycle++ )
    {
        for( tier_cycle_statistics &tcs : _cycle_stats)
        {
            tcs.clear();
        }
        process_simulation_cycle();
        report_cycle_results();
    }
}

