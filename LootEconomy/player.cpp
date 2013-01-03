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
last_upgrade( 0 )
{
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