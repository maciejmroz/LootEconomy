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
market(players),
_step(0),
_cycle(0)
{
    players.resize(config::MAX_PLAYERS);
    rng.seed((unsigned int)std::time(0));
}

simulation::~simulation()
{
}

void simulation::process_simulation_cycle()
{
    for( int i = 0; i < config::MAX_PLAYERS ; i++ )
    {
        _step++;
        players[i].process_simulation_step(*this, i);
        if( _step % config::TRANSACTION_FREQUENCY == 0)
        {
            market.finalize_old_transactions(*this);
        }
    }
}

double simulation::get_average_item_tier()
{
    double sum = 0.0;
    for( const player &p : players)
    {
        sum += p.get_average_tier();
    }
    return sum / players.size();
}

void simulation::begin_reporting()
{
    for(int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"TR_COUNT_T" << i <<"\";";
    }
    for(int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"TR_VOL_T" << i <<"\";";
    }
    for(int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << "\"AVG_PRICE_T" << i <<"\";";
    }
    std::cout << "\"UPG_COUNT\";\"AVG_TIER\"" << std::endl;
}

void simulation::report_cycle_results()
{
    for( int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << cycle_stats[i].successful_transactions << ";";
    }
    for( int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << cycle_stats[i].transaction_volume << ";";
    }
    for( int i = 0; i < config::NUM_ITEM_TIERS; i++ )
    {
        std::cout << market.get_average_tier_price(i) << ";";
    }
    std::cout << std::count_if(players.begin(),players.end(),[&] (const player& p) {return p.last_upgrade == _cycle;}) << ";";
    std::cout << get_average_item_tier() << std::endl;
}

void simulation::run()
{
    _step = 0;
    begin_reporting();
    for( _cycle = 0; _cycle < config::NUM_SIMULATION_CYCLES ; _cycle++ )
    {
        for( tier_cycle_statistics &tcs : cycle_stats)
        {
            tcs.clear();
        }
        process_simulation_cycle();
        report_cycle_results();
    }
}

