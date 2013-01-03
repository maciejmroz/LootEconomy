//
//  item.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 22.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

using namespace economy;

bool item::is_empty() const
{
    return tier == -1;
}

void item::clear()
{
    slot = 0;
    tier = -1;
}
