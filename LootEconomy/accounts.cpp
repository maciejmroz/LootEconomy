//
//  accounts.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 01.01.2013.
//  Copyright (c) 2013 Maciej Mróz. All rights reserved.
//

#include "economy.h"

using namespace economy;

accounts::accounts()
{
    _accounts = new long[MAX_PLAYERS];
    memset( _accounts, 0, MAX_PLAYERS * sizeof(long) );
}

accounts::~accounts()
{
    delete [] _accounts;
    _accounts = NULL;
}

long accounts::get_account(int player_id)
{
    return _accounts[player_id];
}

void accounts::add_to_account(int player_id, long amount)
{
    _accounts[player_id] += amount;
}

void accounts::remove_from_account(int player_id, long amount)
{
    _accounts[player_id] -= amount;
}
