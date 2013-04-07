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
    for ( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        ret += used_items[i].is_empty() ? 0 : used_items[i].tier;
    }
    return ret / NUM_ITEM_SLOTS;
}

void player::generate_items(simulation &sim)
{
    boost::uniform_int<>    it_gen_count_dist(NUM_ITEMS_PER_STEP_MIN,NUM_ITEMS_PER_STEP_MAX);
    
    const int num_items_to_generate = it_gen_count_dist(sim._rng);
    
    for( int i = 0; i < num_items_to_generate; i++ )
    {
        stash.push_back(sim._generator.get_new_item(*this, sim._rng));
    }
}

void player::use_best_items(simulation &sim)
{
    for( item &it : stash )
    {
        item it_copy = it;
        if( used_items[it.slot].tier < it.tier )
        {
            it = used_items[it.slot];
            used_items[it.slot] = it_copy;
            last_upgrade = sim.get_cycle();
        }
    }
    stash.erase(std::remove_if(stash.begin(), stash.end(), [] (const item &it) {return it.is_empty();}), stash.end());
}

void player::destroy_bad_items()
{
    std::sort( stash.begin(), stash.end(),
        [] (const item &i1, const item &i2) {return i1.tier > i2.tier;});
    if( stash.size() <= MAX_ITEMS_TO_ENLIST )
    {
        return;
    }
	account += (currency_t)(stash.size() - MAX_ITEMS_TO_ENLIST) * VENDOR_PRICE;
    stash.erase(stash.begin() + MAX_ITEMS_TO_ENLIST, stash.end());
}

void player::enlist_stash_items(int player_id, simulation &sim)
{
    for( auto &item : stash )
    {
        sim._market.enlist_item(sim.get_step(), item, player_id);
    }
    stash.clear();
}

void player::find_upgrades(int player_id, simulation &sim)
{
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
               ( used_items[worst_item_index].tier > used_items[j].tier) )
            {
                worst_item_index = j;
            }
        }
        if(worst_item_index != -1)
        {
            if( used_items[worst_item_index].tier == NUM_ITEM_TIERS - 1 )
            {
                //player is maxed out
                return;
            }
            offer o;
            if( sim._market.find_offer(worst_item_index, used_items[worst_item_index].tier + 1, account, o) )
            {
                sim._market.bid(o, player_id, o.minimum_bid() );
            }
            processed_slots[worst_item_index] = true;
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
