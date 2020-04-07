#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.hpp>
#include "tokenlock.hpp"

using namespace eosio;

  [[eosio::action]] void tokenlock::refresh(eosio::name username, uint64_t id){
    require_auth(username);
    
    users_index users(_self, _self.value);
    auto exist = users.fidd(username);
    eosio::check(exist == users.end(), "User is already migrated");


  };

  [[eosio::action]] void tokenlock::withdraw(eosio::name username, uint64_t id){
    require_auth(username);

    users_index users(_self, _self.value);
    auto exist = users.fidd(username);
    eosio::check(exist == users.end(), "User is already migrated");
   

  };


  [[eosio::action]] void tokenlock::migrate(eosio::name username){
    require_auth(_self);

    users_index users(_self, _self.value);
    auto exist = users.fidd(username);
    eosio::check(exist == users.end(), "User is already migrated");

    users.emplace(_self, [&](auto u){
      u.username = username;
      u.migrated_at = eosio::time_point_sec(now());
    });

  };


  [[eosio::action]] void tokenlock::add(eosio::name username, uint64_t id, uint64_t parent_id, eosio::time_point_sec datetime, uint64_t algorithm, int64_t amount){
    require_auth(_self);
    
    locks_index locks(_self, username.value);
    history_index history(_self, _self.value);

    //TODO check for user account exist
    eosio::asset amount_in_asset = asset(amount, _op_symbol);

    auto exist = locks.find(id);
    eosio::check(exist == locks.end(), "Lock object with current ID is already exist");

    if (parent_id == 0){ //without parent_id
      eosio::check(amount > 0, "Amount for issue to lock-object should be more then zero");

      if ( algorithm == 0 ){ //transfer unlocked
           
        action(
          permission_level{ _genesis, "active"_n },
          _token_contract, "issue"_n,
          std::make_tuple( username, amount_in_asset, std::string("")) 
        ).send();
          

      } else { //create lock object
        
        locks.emplace(_self, [&](auto &l){
          l.id = id;
          l.datetime = datetime;
          l.algorithm = algorithm;
          l.amount = amount_in_asset;
        });  
    
        action( //issue to genesis account
          permission_level{ _genesis, "active"_n },
          _token_contract, "issue"_n,
          std::make_tuple( _genesis, amount_in_asset, std::string("")) 
        ).send();
          
      }      


    } else { //with parent_id
      //TODO get algorithm for history from parent_id!

      auto parent_lock_object = locks.find(parent_id);
      
      algorithm = parent_lock_object->algorithm;

      eosio::check(parent_lock_object != locks.end(), "Parent lock object is not found");
      print("amount: ", amount);
      
      uint64_t to_retire_amount = uint64_t(0 - amount_in_asset.amount);
      
      print("to retire: ", to_retire_amount);
      eosio::asset to_retire = asset(to_retire_amount, _op_symbol);
      print("to retire: ", to_retire);
      

      eosio::check(amount < 0, "Only the ability to reduce balance is available.");

      eosio::check(parent_lock_object -> amount >= to_retire, "Not enought parent balance for retire");

      locks.modify(parent_lock_object, _self, [&](auto &l){
        l.amount = parent_lock_object -> amount + amount_in_asset;
      });    

      
      action( //burn from genesis account in reduce case
        permission_level{ _genesis, "active"_n },
        _token_contract, "retire"_n,
        std::make_tuple( to_retire, std::string("")) 
      ).send();
    
    }


    history.emplace(_self, [&](auto &l){
      l.id = history.available_primary_key();
      l.lock_id = id;
      l.lock_parent_id = parent_id;
      l.username = username;
      l.datetime = datetime;
      l.algorithm = algorithm;
      l.amount = amount_in_asset;
    });

     
  }


extern "C" {
   
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (code == tokenlock::_self.value) {
          if (action == "add"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::add);
          }        
        } else {
          if (action == "transfer"_n.value){
            
            struct transfer{
                eosio::name from;
                eosio::name to;
                eosio::asset quantity;
                std::string memo;
            };

            auto op = eosio::unpack_action_data<transfer>();

            if (op.to == tokenlock::_self){
              //DISPATCHER FOR INCOME TRANSFERS 
            }
          }
        }
  };
};
