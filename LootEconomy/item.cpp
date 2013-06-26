//
//  item.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 22.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

using namespace economy;

discrete_quality_item::discrete_quality_item()
{
    clear();
}

bool discrete_quality_item::is_empty() const
{
    return tier == -1;
}

void discrete_quality_item::clear()
{
    slot = 0;
    tier = -1;
}

void discrete_quality_item::generate(boost::mt19937 &rng)
{
    boost::uniform_int<>    slot_dist(0,config::NUM_ITEM_SLOTS-1);
    boost::uniform_int<>    percentage_dist(0,99);
    
    slot = slot_dist(rng);
    tier = 0;

    const int promotion_probability = 30;
    
    while ( tier < config::NUM_ITEM_TIERS - 1 )
    {
        if ( percentage_dist(rng) < 100-promotion_probability )
        {
            break;
        }
        tier++;
    }
}

continuous_quality_item::continuous_quality_item()
{
    clear();
}

bool continuous_quality_item::is_empty() const
{
    return tier == -1;
}

void continuous_quality_item::clear()
{
    slot = 0;
    tier = -1.0;
}

void continuous_quality_item::generate(boost::mt19937 &rng)
{
    boost::uniform_int<>    slot_dist(0,config::NUM_ITEM_SLOTS-1);
    boost::uniform_int<>    percentage_dist(0,99);
    
    slot = slot_dist(rng);
    int base_tier = 0;

    const int promotion_probability = 30;
    
    while ( base_tier < config::NUM_ITEM_TIERS - 1 )
    {
        if ( percentage_dist(rng) < 100-promotion_probability )
        {
            break;
        }
        base_tier++;
    }

    tier = base_tier + percentage_dist(rng) / 100.0;
}
