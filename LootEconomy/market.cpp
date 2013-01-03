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

market::market(accounts &acc) :
_current_offer_id(0),
_accounts(acc)
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

void market::enlist_item(long step, const item &it, int seller_id, long min_price)
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
    
    restore_offer_order( od );
}

bool market::find_offer(int slot, int min_tier, long max_price, offer &result)
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

struct find_offer_uid
{
    long ref_uid;
    
    find_offer_uid(long ref_uid_p) :
    ref_uid(ref_uid_p)
    {
    }
    
    bool operator()(const offer &o)
    {
        return o.uid == ref_uid;
    }
};

bool market::bid(const offer &off, int buyer_id, long bid_amount)
{
    offer_data_t &od = _offers[off.it.slot][off.it.tier];
    find_offer_uid pred(off.uid);
    auto iter = std::find_if( od.rbegin(), od.rend(), pred);
    if( iter == od.rend() )
    {
        return false;
    }
    offer &o = *iter;
    if( bid_amount < o.minimum_bid() )
    {
        return false;
    }
    if( bid_amount > _accounts.get_account(buyer_id) )
    {
        return false;
    }
    if( o.has_buyer )
    {
        _accounts.add_to_account(o.buyer_id, o.current_bid);
    }
    o.has_buyer = true;
    o.buyer_id = buyer_id;
    o.current_bid = bid_amount;
    _accounts.remove_from_account(buyer_id, bid_amount);
    if( iter == od.rbegin() )
    {
        restore_offer_order( od );
    }
    else
    {
        std::sort(od.begin(), od.end(), compare_offers_rev() );
    }
    return true;
}

currency_t market::get_suggested_min_price(int slot, int tier)
{
    offer_data_t &od = _offers[slot][tier];
    return std::max( od.empty() ? _last_prices[slot][tier] / 2 : ( od.rbegin()->price() / 2 ),
                    MIN_ENLIST_PRICE );
}

struct offer_age_predicate
{
    long current_step;
    
    bool operator()(const offer &o)
    {
        return o.create_step + MAX_TRANSACTION_AGE < current_step;
    }
};

void market::remove_old_transactions(long step, offer_data_t &od, std::vector<offer> &result)
{
    offer_age_predicate pred;
    pred.current_step = step;
    auto iter = od.begin();
    while( iter != od.end() )
    {
        const offer &o = *iter;
        if( pred( o ) )
        {
            result.push_back( o );
            _last_prices[o.it.slot][o.it.tier] = o.has_buyer ? o.current_bid : o.min_price;
        }
        iter++;
    }
    od.erase( std::remove_if(od.begin(), od.end(), pred), od.end() );
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

currency_t market::get_avg_tier_price(int tier)
{
    int item_count = 0;
    currency_t sum = 0;
    for( int i = 0; i < NUM_ITEM_SLOTS; i++ )
    {
        const offer_data_t &od = _offers[i][tier];
        auto iter = od.begin();
        while( iter != od.end() )
        {
            const offer &o = *iter;
            if( o.has_buyer )
            {
                sum += o.current_bid;
                item_count++;
            }
            iter++;
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
