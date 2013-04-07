//
//  market.cpp
//  LootEconomy
//
//  Created by Maciej Mróz on 22.12.2012.
//  Copyright (c) 2012 Maciej Mróz. All rights reserved.
//

#include "economy.h"

#include <algorithm>
#include <iostream>

using namespace economy;

market::market(players_vec_t &players_vec) :
_current_offer_id(0),
_players(players_vec)
{
    for( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        for( int j = 0; j < NUM_ITEM_TIERS; j++ )
        {
            _last_prices[i][j] = 0;
        }
    }
}

market::~market()
{
}

struct compare_offers_rev
{
    bool operator()(const offer &o1, const offer &o2)
    {
        return o1.price() > o2.price();
    }
};

void market::restore_offer_order(offer_data_t &od)
{
    if( od.size() < 2 )
    {
        return;
    }
    auto iter = od.rbegin();
    offer tmp_o = *iter;
    currency_t ref_price = tmp_o.price();
    
    iter++;
    while( iter != od.rend() )
    {
        if( iter->price() >= ref_price )
        {
            break;
        }
        *(iter-1) = *iter;
        iter++;
    }
    *(iter-1) = tmp_o;
}

void market::enlist_item(long step, const item &it, int seller_id, currency_t min_price)
{
    offer o;
    o.min_price = min_price;
    o.seller_id = seller_id;
    o.it = it;
    o.has_buyer = false;
    o.uid = _current_offer_id++;
    o.create_step = step;
    
    offer_data_t &od = _offers[it.slot][it.tier];
    od.push_back(o);
    
    restore_offer_order(od);
}

void market::enlist_item(long step, const item& it, int seller_id)
{
    enlist_item(step, it, seller_id, get_suggested_minimum_price(it.slot, it.tier));
}

bool market::find_offer(int slot, int min_tier, currency_t max_price, offer &result)
{
    for( int i = NUM_ITEM_TIERS - 1; i >= min_tier; i-- )
    {
        offer_data_t &od = _offers[slot][i];
        if( od.empty() )
        {
            continue;
        }
        offer &o = od.back();
        if( max_price >= o.minimum_bid() )
        {
            result = o;
            return true;
        }
    }
    return false;
}

void market::validate_bid(const offer &o, int buyer_id, currency_t bid_amount)
{
    if( bid_amount < o.minimum_bid() )
    {
        throw bid_amount_too_low_exception();
    }
    if( bid_amount > _players[buyer_id].account )
    {
        throw insufficient_funds_exception();
    }
}

offer& market::find_offer(const offer &off)
{
    auto pred = [&] (const offer &o) {return o.uid == off.uid;};
    offer_data_t &od = _offers[off.it.slot][off.it.tier];
    auto iter = std::find_if( od.rbegin(), od.rend(), pred);
    if( iter == od.rend() )
    {
        throw offer_not_found_exception();
    }
    return *iter;
}

void market::bid(const offer &off, int buyer_id, currency_t bid_amount)
{
    offer &o = find_offer(off);
    validate_bid(o, buyer_id, bid_amount);
    if( o.has_buyer )
    {
        _players[o.buyer_id].account += o.current_bid;
    }
    o.has_buyer = true;
    o.buyer_id = buyer_id;
    o.current_bid = bid_amount;
    _players[buyer_id].account -= bid_amount;

    offer_data_t &od = _offers[o.it.slot][o.it.tier];
    if( o.uid == od.rbegin()->uid )
    {
        restore_offer_order(od);
    }
    else
    {
        std::sort(od.begin(), od.end(), compare_offers_rev());
    }
}

currency_t market::get_suggested_minimum_price(int slot, int tier)
{
    offer_data_t &od = _offers[slot][tier];
    return std::max(od.empty() ? _last_prices[slot][tier] / 2 : ( od.rbegin()->price() / 2 ),
                    MIN_ENLIST_PRICE);
}

void market::remove_old_transactions(long step, offer_data_t &od, std::vector<offer> &result)
{
    auto pred = [&] (const offer &o) {return o.create_step + MAX_TRANSACTION_AGE < step;};
    for( const auto &o : od )
    {
        if( pred(o) )
        {
            result.push_back(o);
            _last_prices[o.it.slot][o.it.tier] = o.has_buyer ? o.current_bid : o.min_price;
        }
    }
    od.erase(std::remove_if(od.begin(), od.end(), pred), od.end());
}

void market::remove_old_transactions(long step, std::vector<offer> &result)
{
    for( int slot = 0; slot < NUM_ITEM_SLOTS; slot++ )
    {
        for( int tier = 0; tier < NUM_ITEM_TIERS; tier++ )
        {
            remove_old_transactions( step, _offers[slot][tier], result );
        }
    }
}

currency_t market::get_average_tier_price(int tier)
{
    int item_count = 0;
    currency_t sum = 0;
    for( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        const offer_data_t &od = _offers[i][tier];
        for( const auto &o : od )
        {
            if( o.has_buyer )
            {
                sum += o.current_bid;
                item_count++;
            }
        }
    }
    if( item_count )
    {
        return sum / item_count;
    }
    //try to average last known transaction prices
    item_count = 0;
    sum = 0;
    for( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        if( _last_prices[i][tier] != 0 )
        {
            sum += _last_prices[i][tier];
            item_count++;
        }
    }
    if( item_count )
    {
        return sum / item_count;
    }
    return MIN_ENLIST_PRICE;
}
