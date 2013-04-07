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
    _accounts = new currency_t[MAX_PLAYERS];
    memset( _accounts, 0, MAX_PLAYERS * sizeof(currency_t) );
}

accounts::~accounts()
{
    delete [] _accounts;
    _accounts = NULL;
}

currency_t accounts::get_account(int player_id)
{
    return _accounts[player_id];
}

void accounts::add_to_account(int player_id, currency_t amount)
{
    _accounts[player_id] += amount;
}

void accounts::remove_from_account(int player_id, currency_t amount)
{
    _accounts[player_id] -= amount;
}
