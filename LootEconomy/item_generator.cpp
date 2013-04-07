//
//  item_generator.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 22.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

using namespace economy;

item_generator::item_generator()
{
}

int item_generator::get_promotion_probability(const player &p)
{
    const struct avg_tbl_s
    {
        float   avg;
        int     p;
    } avg_tbl[] =
    {
        {8.0f,30},
        {6.0f,25},
        {4.0f,20},
        {2.0f,15},
        {0.0f,10},
    };
    const int avg_tbl_size = sizeof(avg_tbl) / sizeof(avg_tbl_s);
    const double avg_tier = p.get_average_tier();
    
    for ( int i = 0; i < avg_tbl_size; i++ )
    {
        if( avg_tbl[i].avg >= avg_tier )
        {
            return avg_tbl[i].p;
        }
    }
    
    return 0;
}

item item_generator::get_new_item(const player &p, boost::mt19937 &rng)
{
    boost::uniform_int<>    slot_dist(0,NUM_ITEM_SLOTS-1);
    boost::uniform_int<>    percentage_dist(0,99);
    
    item it;
    it.clear();
    it.slot = slot_dist(rng);
    it.tier = 0;

    const int promotion_probability = get_promotion_probability(p);
    
    while ( it.tier < NUM_ITEM_TIERS - 1 )
    {
        if ( percentage_dist(rng) < 100-promotion_probability )
        {
            break;
        }
        it.tier++;
    }
    
    return it;
}