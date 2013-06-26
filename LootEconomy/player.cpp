//
//  player.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 22.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

using namespace economy;

player::player() :
last_upgrade(0),
account(0)
{
}

player::player(const player &p)
{
    operator=(p);
}

void player::operator=(const player &p)
{
    used_items = p.used_items;
    stash = p.stash;
    last_upgrade = p.last_upgrade;
    account = p.account;
}

double player::get_average_tier() const
{
    double ret = 0.0;
    for ( int i = 0; i < config::NUM_ITEM_SLOTS; i++ )
    {
        ret += used_items[i].is_empty() ? 0 : used_items[i].tier;
    }
    return ret / config::NUM_ITEM_SLOTS;
}

void player::generate_items(simulation &sim)
{
    boost::uniform_int<>    it_gen_count_dist(config::NUM_ITEMS_PER_STEP_MIN,config::NUM_ITEMS_PER_STEP_MAX);
    
    const int num_items_to_generate = it_gen_count_dist(sim.rng);
    
    for( int i = 0; i < num_items_to_generate; i++ )
    {
        item it;
        it.generate(sim.rng);
        stash.push_back(it);
    }
}

void player::use_best_items(simulation &sim)
{
    for( item &it : stash )
    {
        if( used_items[it.slot].tier < it.tier )
        {
            std::swap(it, used_items[it.slot]); 
            last_upgrade = sim.get_cycle();
        }
    }
    stash.erase(std::remove_if(stash.begin(), stash.end(),
        [] (const item &it) {return it.is_empty();}), stash.end());
}

void player::destroy_bad_items()
{
    std::sort( stash.begin(), stash.end(),
        [] (const item &i1, const item &i2) {return i1.tier > i2.tier;});
    if( stash.size() <= config::MAX_ITEMS_TO_ENLIST )
    {
        return;
    }
	account += (currency_t)(stash.size() - config::MAX_ITEMS_TO_ENLIST) * config::VENDOR_PRICE;
    stash.erase(stash.begin() + config::MAX_ITEMS_TO_ENLIST, stash.end());
}

void player::enlist_stash_items(int player_id, simulation &sim)
{
    for( auto &item : stash )
    {
        sim.market.enlist_item(sim.get_step(), item, player_id);
    }
    stash.clear();
}

void player::find_upgrades(int player_id, simulation &sim)
{
    bool processed_slots[config::NUM_ITEM_SLOTS];
    
    for( int i = 0; i < config::NUM_ITEM_SLOTS; i++ )
    {
        processed_slots[i] = false;
    }
    
    for(int i = 0; i < config::NUM_ITEM_SLOTS ; i++ )
    {
        int worst_item_slot = -1;
        for( int j = 0; j < config::NUM_ITEM_SLOTS; j++ )
        {
            if( processed_slots[j] )
            {
                continue;
            }
            if( ( worst_item_slot == -1 ) ||
               ( used_items[worst_item_slot].tier > used_items[j].tier) )
            {
                worst_item_slot = j;
            }
        }
        if(worst_item_slot != -1)
        {
            if( used_items[worst_item_slot].tier == config::NUM_ITEM_TIERS - 1 )
            {
                //player is maxed out
                return;
            }
            offer o;
            if( sim.market.find_offer(worst_item_slot, used_items[worst_item_slot].tier + 1, account, o) )
            {
                sim.market.bid(o, player_id, o.minimum_bid() );
            }
            processed_slots[worst_item_slot] = true;
        }
    }
}

void player::process_simulation_step(simulation &sim, int player_id)
{
    generate_items(sim);
    use_best_items(sim);
    destroy_bad_items();
    enlist_stash_items(player_id, sim);
    find_upgrades(player_id, sim);
}
