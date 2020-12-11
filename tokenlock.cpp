
#include "tokenlock.hpp"

using namespace eosio;

  [[eosio::action]] void tokenlock::setbal(eosio::name username, eosio::asset cru_balance){
    require_auth(_self);

    tokenlock::modify_balance(username, cru_balance, 0);

  }

  [[eosio::action]] void tokenlock::updatebal(eosio::name username){
    require_auth(_updater);
  }

  double tokenlock::get_current_percent(uint64_t last_distributed_cycle){
    if (last_distributed_cycle >= 1 && last_distributed_cycle < 7 ) //6m
      return 1;
    if (last_distributed_cycle >= 7 && last_distributed_cycle < 13 ) //6m
      return 1.5;
    if (last_distributed_cycle >= 13 && last_distributed_cycle < 19 ) //6m
      return 2;
    if (last_distributed_cycle >= 19 && last_distributed_cycle < 25 ) //6m
      return 3;
    if (last_distributed_cycle >= 25 && last_distributed_cycle < 36 ) //11m
      return 5;

    return 0;
  }


void tokenlock::chlbal(eosio::name username, eosio::asset balance, uint64_t type){
  eosio::check(has_auth(tokenlock::_token_contract) || has_auth(tokenlock::_system_contract) || has_auth(tokenlock::_staker_contract), "missing required authority");

  tokenlock::modify_balance(username, balance, type);
}

void tokenlock::modify_balance(eosio::name username, eosio::asset balance, uint64_t type) {
    //types: 0 - change only total, 1 - change only frozen, 2 - change only staked
    
    tokenlock::tbalance_index balances(tokenlock::_self, tokenlock::_self.value);
    tokenlock::tbalance2_index balances2(tokenlock::_self, tokenlock::_self.value);
    
    auto bal = balances.find(username.value);
    auto bal2 = balances2.find(username.value);
    
    if ((balance.symbol == _wcru_symbol || balance.symbol == _cru_symbol) && type == 2){
      if (bal2 != balances2.end()){
        balances2.modify(bal2, _self, [&](auto &b){
          if (balance.symbol == _wcru_symbol) 
            b.wcru_staked += balance;

          if (balance.symbol == _cru_symbol) 
            b.cru_staked += balance;
        });
      } else {
        balances2.emplace(_self, [&](auto &b) {
          b.username = username;
          
          if (balance.symbol == _wcru_symbol){
            b.cru_staked = asset(0, _cru_symbol);
            b.wcru_staked = balance; 
          };

          if (balance.symbol == _cru_symbol){
            b.cru_staked = balance;
            b.wcru_staked = asset(0, _wcru_symbol); 
          };
        });

      }
    } else if (bal != balances.end()){

      balances.modify(bal, tokenlock::_self, [&](auto &b){
        if (balance.symbol == _cru_symbol){
          if (type == 0){
            b.cru_total += balance;
          } else if (type == 1) {
            b.cru_frozen += balance;
          } 
        }  else if (balance.symbol == _wcru_symbol){
          if (type == 0){
            b.wcru_total += balance;
          } else if (type == 1) {
            b.wcru_frozen += balance;
          } 
        } else if (balance.symbol == _untb_symbol){
          if (type == 0){
            b.untb_total += balance;
          } else if (type == 1) {
            b.untb_total += balance;
            b.untb_staked += balance;
          } 
        } else if (balance.symbol == _usdu_symbol){
          if (type == 0){
            b.usdu_total += balance;
          } 
        }
      });  

    } else {
      balances.emplace(tokenlock::_self, [&](auto &b){
        b.username = username;
        
        if (balance.symbol == _cru_symbol){
          if (type == 0){
            b.cru_total = balance;
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = asset(0, _untb_symbol);
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = asset(0, _usdu_symbol);

            
          } else if (type == 1) {
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = balance;
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = asset(0, _untb_symbol);
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = asset(0, _usdu_symbol);

          } 
        } else if (balance.symbol == _wcru_symbol) {
          if (type == 0){
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = balance;
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = asset(0, _untb_symbol);
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = asset(0, _usdu_symbol);
            
          } else if (type == 1) {
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = balance;
            b.untb_total = asset(0, _untb_symbol);
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = asset(0, _usdu_symbol);
            
          } 
        } else if (balance.symbol == _untb_symbol){

          if (type == 0){
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = balance;
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = asset(0, _usdu_symbol);
            
          } else if (type == 1) {
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = balance;
            b.untb_staked = balance;
            b.usdu_total = asset(0, _usdu_symbol);
            
          } 
        } else if (balance.symbol == _usdu_symbol){

          if (type == 0){
            b.cru_total = asset(0, _cru_symbol);
            b.cru_frozen = asset(0, _cru_symbol);
            b.wcru_total = asset(0, _wcru_symbol);
            b.wcru_frozen = asset(0, _wcru_symbol);
            b.untb_total = asset(0, _untb_symbol);
            b.untb_staked = asset(0, _untb_symbol);
            b.usdu_total = balance;
            
          } 
        }
      });
    
    }
}


   [[eosio::action]] void tokenlock::refresh(eosio::name username, uint64_t id){
    // require_auth(username); //any
      tokenlock::refresh_action(username, id);
    };

  /*  
   *  refresh(eosio::name username, uint64_t id)
   *    - eosio::name username - пользователь, которому принадлежит обновляемый объект начисления
   *    - uint64_t id - целочисленный идентификатор объекта начисления
   *    
   *    Авторизация:
   *      - any@active

   *    Обновляет состояние объекта начисления пользователя. 
   *    Производит расчет доступных к выводу токенов и записывает их в поле available таблицы locks. 
   *
   */
  void tokenlock::refresh_action(eosio::name username, uint64_t id){
    // require_auth(username); //any
    
    users_index users(_self, _self.value);
    auto user = users.find(username.value);
    eosio::check(user != users.end(), "User is not migrated");

    eosio::check(user -> unfreeze_enabled == true, "Unfreeze is not enabled yet");


    locks_index locks(_self, username.value);
    auto lock = locks.find(id);
    eosio::check(lock != locks.end(), "Lock object is not found");
    // eosio::check(lock->is_staked == false, "Lock object cannot be refreshed before unstake");

    auto start_unfreeze_at = user -> start_unfreeze_at;

    auto created_at = lock-> created;
    eosio::time_point_sec distribution_start_at;
    uint64_t freeze_seconds;
    
    eosio::check(lock ->algorithm == 1 || lock->algorithm == 2 || lock -> algorithm == 3, "Not possible to refresh not registered algorithm");
    
    if (lock->algorithm == 1){
      freeze_seconds = _alg1_freeze_seconds;
    } else if (lock->algorithm == 2) {
      freeze_seconds = _alg2_freeze_seconds;      
    } else if (lock->algorithm == 3) {
      freeze_seconds = _alg3_freeze_seconds;      
    };
    
    distribution_start_at = eosio::time_point_sec(created_at.sec_since_epoch() + freeze_seconds);
    
    bool distribution_is_start = distribution_start_at <= eosio::time_point_sec(now());
    
    print("distribution_is_start:", distribution_is_start, ";");

    uint64_t last_distributed_cycle;
    uint64_t compressed_cycles;

    if (distribution_is_start) {

      if (lock -> last_distribution_at == eosio::time_point_sec(0)){ //Распределяем по контракту впервые
        
        uint64_t cycle_distance = start_unfreeze_at.sec_since_epoch() >= created_at.sec_since_epoch() ? (start_unfreeze_at.sec_since_epoch() - created_at.sec_since_epoch()) /  _cycle_length : 0;
        uint64_t freeze_cycles = freeze_seconds / _cycle_length;

        last_distributed_cycle = 0;
        compressed_cycles = cycle_distance >= freeze_cycles ? cycle_distance - freeze_cycles : 0;
      
      } else { //Если распределение уже производилось

        last_distributed_cycle = ((lock -> last_distribution_at).sec_since_epoch() - created_at.sec_since_epoch()) / _cycle_length - freeze_seconds / _cycle_length;

      }

      print("last_distributed_cycle1:", last_distributed_cycle, ";");
      
      uint64_t between_now_and_created_in_seconds = now() - created_at.sec_since_epoch() - freeze_seconds;
      uint64_t current_cycle = between_now_and_created_in_seconds / _cycle_length;
      
      double percent = 0;
      
      print("current_cycle:", current_cycle, ";");

      eosio::asset to_user_summ = asset(0, lock->amount.symbol);
      eosio::asset asset_amount_to_unfreeze_summ = asset(0, lock->amount.symbol);
      eosio::asset asset_amount_already_unfreezed_summ = asset(0, lock->amount.symbol);

      while ( last_distributed_cycle < current_cycle && last_distributed_cycle < 36 && lock -> amount >= lock->withdrawed + lock->available ) {

        last_distributed_cycle++;

        double percent = get_current_percent(last_distributed_cycle);

        if (last_distributed_cycle < 36){
          double amount_to_unfreeze = 0;
          double amount_already_unfreezed = 0;
          double last_part = 0;
          eosio::asset asset_last_part = asset(0, lock->amount.symbol);
          
          if (last_distributed_cycle <= compressed_cycles ){
          
            amount_already_unfreezed = percent * (lock -> amount).amount / 100;
            
          } else {
          
            amount_to_unfreeze = percent * (lock -> amount).amount / 100;
          
          }

          eosio::asset asset_amount_to_unfreeze = asset((uint64_t)amount_to_unfreeze, lock->amount.symbol);
          eosio::asset asset_amount_already_unfreezed = asset((uint64_t)amount_already_unfreezed, lock->amount.symbol);
          
          eosio::asset asset_back_to_reserve = asset(0, lock->amount.symbol);

          if (amount_already_unfreezed > 0){

            eosio::asset to_user_already_unfreezed = asset_amount_already_unfreezed;
            eosio::asset debt_in_asset = get_debt(username);
          
            if (debt_in_asset.amount != 0 && lock->amount.symbol == _cru_symbol){
              if (to_user_already_unfreezed.amount >= debt_in_asset.amount){
                asset_back_to_reserve = debt_in_asset;
                to_user_already_unfreezed = to_user_already_unfreezed - asset_back_to_reserve;
              } else {
                asset_back_to_reserve = to_user_already_unfreezed;
                to_user_already_unfreezed = asset(0, lock->amount.symbol);
              }
              
              modify_debt(username, asset_back_to_reserve);

              if (asset_back_to_reserve.amount > 0)
                action(
                    permission_level{ _self, "active"_n },
                    _token_contract, "transfer"_n,
                    std::make_tuple( _self, _reserve, asset_back_to_reserve, std::string("")) 
                ).send(); 

            }
            

            if (to_user_already_unfreezed.amount > 0){
              action(
                  permission_level{ _self, "active"_n },
                  _token_contract, "transfer"_n,
                  std::make_tuple( _self, username, to_user_already_unfreezed, std::string("")) 
              ).send();        
            }

            locks.modify(lock, _self, [&](auto &l){
              l.last_distribution_at = eosio::time_point_sec((lock->created).sec_since_epoch() + last_distributed_cycle * _cycle_length + freeze_seconds);
              l.withdrawed += asset_amount_already_unfreezed;
            });

          } 

          if (last_distributed_cycle == 35){
            asset_amount_to_unfreeze = lock -> amount - lock -> withdrawed - lock->available;
          }

          modify_balance(username, - asset_amount_already_unfreezed + asset_back_to_reserve, 0);
          modify_balance(username, - asset_amount_already_unfreezed, 1);
          
          print("asset_amount_to_unfreeze: ", asset_amount_to_unfreeze);
          
          eosio::check(lock->amount >= lock->withdrawed + lock -> available, "system error");

          auto sediment = lock->amount - lock->withdrawed - lock -> available;
          
          print("sediment: ", sediment);

          locks.modify(lock, _self, [&](auto &l){
            if (lock -> available + lock -> withdrawed + asset_amount_to_unfreeze > lock->amount)
              l.available += sediment;
            else 
              l.available += asset_amount_to_unfreeze;

            l.last_distribution_at = eosio::time_point_sec((lock->created).sec_since_epoch() + last_distributed_cycle * _cycle_length + freeze_seconds);
            // l.withdrawed += asset_amount_already_unfreezed;
          });
        }
      }
    }
  };


  void tokenlock::modify_debt(eosio::name username, eosio::asset amount_to_add){

    debts_index debts(_self, username.value);
    auto debt = debts.find(username.value);

    if (debt == debts.end()){

      debts.emplace(_self, [&](auto &d){
        d.username = username;
        d.amount = amount_to_add;
      });

    } else {
      debts.modify(debt, _self, [&](auto &d){
        d.amount += amount_to_add;
      });
  
    }
    
    dhistory_index dhistory(_self, username.value);
    
    dhistory.emplace(_self, [&](auto &dh){
      dh.id = dhistory.available_primary_key();
      dh.amount = amount_to_add;
      dh.timestamp = eosio::time_point_sec(now());
    });

  }

  eosio::asset tokenlock::get_debt(eosio::name username){
    
    debts_index debts(_self, username.value);
    auto debt = debts.find(username.value);
    eosio::asset debt_in_asset;

    if (debt != debts.end()){
      debt_in_asset = debt -> amount;
    } else {
      debt_in_asset = asset(0, _cru_symbol);
    }

    eosio::asset positive_debt_in_asset = asset((uint64_t)(0 - debt_in_asset.amount), debt_in_asset.symbol);

    return positive_debt_in_asset;

  }

  [[eosio::action]] void tokenlock::frstake(eosio::name username, eosio::asset quantity) {
    require_auth(username);

    tbalance_index tbalances(_self, _self.value);
    // tbalance2_index tbalances2(_self, _self.value);
    auto tbalance = tbalances.find(username.value);
    // auto tbalance2 = tbalances2.find(username.value);
    
    // uint64_t staked_amount = tbalance2 == tbalances2.end() ? 0 : tbalance2 -> wcru_staked.amount;
    eosio::check(quantity.amount > 0, "Only positive amounts can be staked");
    eosio::check(quantity.symbol == _wcru_symbol, "Only frozen WCRU can be staked");

    eosio::check(quantity.amount <= tbalance -> wcru_frozen.amount, "Not enought frozen balance for stake");

    modify_balance(username, - quantity, 1);
    modify_balance(username, quantity, 2);
  
    action(
        permission_level{ tokenlock::_self, "active"_n },
        "eosio"_n, "frstake"_n,
        std::make_tuple( username, quantity) 
    ).send(); 

  }

  [[eosio::action]] void tokenlock::frunstake(eosio::name username, eosio::asset quantity) {
    require_auth(username);

    tokenlock::intfrunstake(username, quantity);

  }

  void tokenlock::intfrunstake(eosio::name username, eosio::asset quantity) {
    tbalance2_index tbalances2(_self, _self.value);
    auto tbalance2 = tbalances2.find(username.value);
    
    
    if (tbalance2 != tbalances2.end()) {
      eosio::check(quantity.amount > 0, "Only positive amounts can be unstaked");
      eosio::check(quantity.symbol == _wcru_symbol, "Only frozen WCRU can be unstaked by this method");
      eosio::check(tbalance2 -> wcru_staked >= quantity, "Not enought balance for unstake");
      
      modify_balance(username, - quantity, 2);
      modify_balance(username,   quantity, 1);

      action(
          permission_level{ tokenlock::_self, "active"_n },
          "eosio"_n, "frunstake"_n,
          std::make_tuple( username, quantity, std::string("")) 
      ).send();    
    }
    
  }

  [[eosio::action]] void tokenlock::intchange(eosio::name username, uint64_t lock_id, eosio::asset quantity, uint64_t est_converted_month){
      require_auth(username);

      tokenlock::users_index users(tokenlock::_self, tokenlock::_self.value);
      auto exist = users.find(username.value);
      eosio::check(exist != users.end(), "User is not migrated");
      
      eosio::check(quantity.symbol == _cru_symbol, "Only CRU can be changed by this method to WCRU");
      tokenlock::locks_index locks(tokenlock::_self, username.value);
      auto lock = locks.find(lock_id);

      eosio::check(lock->available.symbol == _cru_symbol, "Only CRU can be changed by this method to WCRU");
      
      eosio::check(est_converted_month > 0, "Converted months should be more then 0");

      tokenlock::refresh_action(username, lock_id);

      if (lock->available.amount > 0)
        action(
            permission_level{ tokenlock::_self, "active"_n },
            tokenlock::_token_contract, "transfer"_n,
            std::make_tuple( tokenlock::_self, username, lock->available, std::string("")) 
        ).send(); 

      eosio::check(lock->amount >= lock->withdrawed + lock->available, "system error on change");

      eosio::asset remain_cru = lock->amount - lock->withdrawed - lock->available;
      
      eosio::check(remain_cru >= quantity, "Not enought lock balance for change");

      locks.modify(lock, _self, [&](auto &l){
        l.withdrawed += quantity;
      });

      //TODO fix bug

      tokenlock::locks_index3 locks3(_self, username.value);
      auto lock3 = locks3.find(lock_id);
      
      if (lock3 == locks3.end()) {
        locks3.emplace(_self, [&](auto &l){
          l.id = lock_id;
          l.converted = quantity;
          l.est_converted_month = est_converted_month;
        });
      } else {
        locks3.modify(lock3, _self, [&](auto &l){
          l.converted += quantity;
          l.est_converted_month += est_converted_month;
        });
      };


      action(
          permission_level{ tokenlock::_self, "active"_n },
          _token_contract, "transfer"_n,
          std::make_tuple( _self, _interchange, quantity, std::string("")) 
      ).send(); 

      action(
          permission_level{ tokenlock::_self, "active"_n },
          tokenlock::_interchange, "intchange"_n,
          std::make_tuple( username, quantity, lock->id, std::string("")) 
      ).send(); 
 

      modify_balance(username, - quantity, 0);
      modify_balance(username, - quantity, 1);
      
      eosio::asset wcru_quantity = asset(quantity.amount, _wcru_symbol);
      // print("wcru_quantity: ", wcru_quantity);
      // modify_balance(username, wcru_quantity, 0);
      // modify_balance(username, wcru_quantity, 1);

  }



  /*  
   *  withdraw(eosio::name username, uint64_t id)
   *    - username - пользователь, которому принадлежит выводимый объект начисления
   *    - id - идентификатор объекта начисления
   *    
   *    Требует авторизации ключом username уровня active.
   *    Метод вывода баланса объекта начисления
   *    Передает размороженные токены объекта начисления на аккаунт пользователя. 
   *    Уменьшает количество доступных токенов в поле available таблицы locks до нуля. 
   *    Увеличивает количество выведенных токенов в поле withdrawed на сумму вывода. 
   *    При полном выводе всех токенов - удаляет объект начисления.
   */

  [[eosio::action]] void tokenlock::withdraw(eosio::name username, uint64_t id){
    require_auth(username);

    users_index users(_self, _self.value);
    auto exist = users.find(username.value);
    eosio::check(exist != users.end(), "User is not migrated");
    
    locks_index locks(_self, username.value);
    auto lock = locks.find(id);

    eosio::check(lock != locks.end(), "Change object is not found");

    eosio::asset to_user;
    eosio::asset asset_back_to_reserve;


    if (lock-> available.symbol == _wcru_symbol){
        eosio::check(lock->is_staked == false, "Cannot withdraw staked lock object");
    };

    if ((lock -> available).amount > 0){
      
      eosio::asset to_withdraw = lock -> available;
      
      if (lock->available.symbol == _cru_symbol){
        eosio::asset debt_in_asset = get_debt(username);
        asset_back_to_reserve = asset(0, _cru_symbol);
        to_user = asset(0, _cru_symbol);
        
        if (debt_in_asset.amount != 0){

          if (to_withdraw.amount >= debt_in_asset.amount){
            asset_back_to_reserve = debt_in_asset;
            to_user = to_withdraw - asset_back_to_reserve;
          } else {
            asset_back_to_reserve = to_withdraw;
            to_user = asset(0, _cru_symbol);
          }
          print("asset_back_to_reserve:", asset_back_to_reserve, ";");

          modify_debt(username, asset_back_to_reserve);

          if (asset_back_to_reserve.amount > 0)
            action(
                permission_level{ _self, "active"_n },
                _token_contract, "transfer"_n,
                std::make_tuple( _self, _reserve, asset_back_to_reserve, std::string("")) 
            ).send(); 

        } else {
          to_user = to_withdraw;
        }
      } else {
        to_user = to_withdraw;
      }
      

      if (to_user.amount > 0)
        action(
            permission_level{ _self, "active"_n },
            _token_contract, "transfer"_n,
            std::make_tuple( _self, username, to_user, std::string("")) 
        ).send();        


      print("to_user:", to_user, ";");
      
     

      eosio::check(lock->withdrawed + to_withdraw <= lock -> amount, "system error");
      
      locks.modify(lock, _self, [&](auto &l){
          l.available = asset(0, lock->amount.symbol);
          l.withdrawed += to_withdraw; 
      });

      if (to_user.symbol == _wcru_symbol) {
        tbalance2_index tbalances2(_self, _self.value);
        auto tbalance2 = tbalances2.find(username.value);
        if (tbalance2 -> wcru_staked < to_user)
          tokenlock::intfrunstake(username, tbalance2 -> wcru_staked);
      }

      modify_balance(username, - to_user, 0);
      modify_balance(username, - to_user, 1);
      


    } else {
      eosio::check(false, "Not possible to withdraw a zero amount");
    }


  };


  /*
   *  migrate(eosio::name username)
   *    - username - имя аккаунта пользователя
   *    
   *    Требует авторизации ключом аккаунта tokenlock уровня active.
   *    Событие миграции активирует разморозку по контракту tokenlock. 
   *    До события миграции пользователь не может совершать действия со своими объектами начисления. 
   *    Разморозка токенов по контракту начинается в следующем цикле распределения после события миграции.
   *    
   *    Пример:
   *      bob получил первый объект начисления по базе данных 240 дней назад. 
   *      Он уже получил две выплаты согласно алгоритму 1 по базе данных. 
   *      После вызова события миграции для аккаунта bob, ему будет доступно только 34 цикла распределения в контракте.
   *      Сумма, которая уже была получена пользователем по базе данных до события миграции рассчитывается и устанавливается в поле withdrawed при первом обновлении объекта начисления. 
   */
  [[eosio::action]] void tokenlock::migrate(eosio::name username, eosio::public_key public_key, bool self_owner = false){
    require_auth(_self);
    
    users_index users(_self, _self.value);
    auto exist = users.find(username.value);
    eosio::check(exist == users.end(), "User is already migrated");

    users.emplace(_self, [&](auto &u){
      u.username = username;
      u.migrated_at = eosio::time_point_sec(now());
      u.unfreeze_enabled = true;
      u.start_unfreeze_at = eosio::time_point_sec(now());
    });


    //замена активного ключа
    authority newauth;
    newauth.threshold = 1;
    
    key_weight keypermission{public_key, 1};
    newauth.keys.emplace_back(keypermission);

    eosio::action(eosio::permission_level(username, eosio::name("owner")), 
      eosio::name("eosio"), eosio::name("updateauth"), std::tuple(username, 
        eosio::name("active"), eosio::name("owner"), newauth) 
    ).send();
    
    if (self_owner == true){

      eosio::action(eosio::permission_level(username, eosio::name("owner")), 
        eosio::name("eosio"), eosio::name("updateauth"), std::tuple(username, 
          eosio::name("owner"), eosio::name(""), newauth) 
      ).send();
    }

  };


  /*
   *  restore(eosio::name username, eosio::public_key public_key, bool self_owner = false)
   *    - username - имя аккаунта пользователя
   *    - public_key - публичный ключ
   *    - self_owner - флаг самостоятельного владения аккаунтом

   *    Требует авторизации ключом аккаунта restore уровня active.
   *    Изменяет активный ключ пользователя и/или передает права владельца аккаунтом пользователю.
   */
  [[eosio::action]] void tokenlock::restore(eosio::name username, eosio::public_key public_key, bool self_owner = false){
    require_auth(_self);
    
    users_index users(_self, _self.value);
    auto exist = users.find(username.value);
    eosio::check(exist != users.end(), "User is not migrated");

    //замена активного ключа
    authority newauth;
    newauth.threshold = 1;
    
    key_weight keypermission{public_key, 1};
    newauth.keys.emplace_back(keypermission);

    eosio::action(eosio::permission_level(username, eosio::name("owner")), 
      eosio::name("eosio"), eosio::name("updateauth"), std::tuple(username, 
        eosio::name("active"), eosio::name("owner"), newauth) 
    ).send();
    
    if (self_owner == true){

      eosio::action(eosio::permission_level(username, eosio::name("owner")), 
        eosio::name("eosio"), eosio::name("updateauth"), std::tuple(username, 
          eosio::name("owner"), eosio::name(""), newauth) 
      ).send();
    }

  };


  /*
   *  add (eosio::name username, uint64_t id, uint64_t parent_id, eosio::time_point_sec datetime, uint64_t algorithm, int64_t amount)
   *    - username - имя пользователя
   *    - id - идентификатор начисления во внешней базе данных
   *    - parent_id - идентификатор баланса списания во внешней базе данных. Указывается с отрицательной суммой amount для списания из объекта начисления
   *    - datetime - дата создания объекта во внешней базе данных в формате "2020-04-08T16:11:22"
   *    - algorithm - используемый алгоритм разморозки. Принимает значения 0 - размороженные токены, необходимо сразу перечислить; 1 - алгоритм разморозки 1, 2 - алгоритм разморозки 2.
   *    - amount - целочисленная величина начисления или списания CRU. Может принимать отрицательные знания при списании. 
   *    
   *    Требует авторизации ключом аккаунта tokenlock уровня active.
   *    Добавляет объект начисления в таблицу locks. 
   *    Списывает токены из объекта начисления parent_id, если parent_id != 0 и amount < 0
   *    Инициирует эмиссию токена в контракте eosio.token на аккаунт reserve с целью Mдальнейшей передачи на аккаунт пользователя username по событию withdraw или немедленно. 
   *    Размороженные объекты немедленно передаются пользователю без записи в таблицу locks. Замороженные объекты требуют совершения действия пользователя для получения размороженных токенов.
   */

  [[eosio::action]] void tokenlock::add(eosio::name username, uint64_t id, uint64_t parent_id, eosio::time_point_sec datetime, uint64_t algorithm, int64_t amount){
    require_auth(_self);
    
    eosio::check(algorithm == 0 || algorithm == 1 || algorithm == 2 || algorithm == 3, "Not possible to add transaction with current algorithm");

    locks_index locks(_self, username.value);
    history_index history(_self, _self.value);
    eosio::asset amount_in_asset;

    if (algorithm != 3){
      auto history_by_id_idx = history.template get_index<"byid"_n>();
      auto exist_hist = history_by_id_idx.find(id);
      eosio::check(exist_hist == history_by_id_idx.end(), "Operation with current ID is already exist");
    
      users_index users(_self, _self.value);
      auto user = users.find(username.value);
      eosio::check(user == users.end(), "User is already migrated. Any transactions from this method is prohibited.");
      amount_in_asset = asset(amount, _cru_symbol);
    } else {
      amount_in_asset = asset(amount, _wcru_symbol);
    }

    
    print("username:", username, ";");
    print("id:", id, ";");
    print("parent_id:", parent_id, ";");
    print("algorithm:", algorithm, ";");
    print("amount:", amount, ";");

    
    auto exist_lock = locks.find(id);
    eosio::check(exist_lock == locks.end(), "Lock object with current ID is already exist");

      
    if (parent_id == 0){ //без parent_id
      
      if ( algorithm == 0 ){ //выпуск токенов на пользователя или перевод от пользователя в случае покупки для unlocked CRU
          eosio::check(amount_in_asset.symbol == _cru_symbol, "Only CRU can be transfered with zero-algorithm");
          debts_index debts(_self, username.value);
          auto debt = debts.find(username.value);
            
          if (amount < 0){

            // Раскоментировать, если после миграции создание долга для пользователя будет запрещено, 
            // а все покупки будут происходить только через контракты на блокчейне.
            // users_index users(_self, _self.value); 
            // auto user = users.find(username.value);
            // eosio::check(user == users.end(), "Cant create debt after migration");


            modify_debt(username, amount_in_asset);
            
            modify_balance(username, amount_in_asset, 0);
            
            print("debt_added:", amount_in_asset, ";");
          
          } else if (amount > 0) {

            eosio::asset to_user = amount_in_asset;

            if (to_user.amount > 0)
               action(
                 permission_level{ _reserve, "active"_n },
                 _token_contract, "transfer"_n,
                 std::make_tuple( _reserve, username, to_user, std::string("")) 
               ).send();
          
          }
      
      } else { //создаем объект заморозки
        eosio::check(amount_in_asset.amount > 0, "Only positive amounts can be added with non-zero algorithm");

        locks.emplace(_self, [&](auto &l){
          l.id = id;
          l.created = datetime;
          l.last_distribution_at = eosio::time_point_sec(0);
          l.algorithm = algorithm;
          l.amount = amount_in_asset;
          l.available = asset(0, amount_in_asset.symbol);
          l.withdrawed = asset(0, amount_in_asset.symbol);
          l.staked = asset(0, amount_in_asset.symbol);
        });  
        
        modify_balance(username, amount_in_asset, 0);
        modify_balance(username, amount_in_asset, 1);

        if (amount_in_asset.symbol == _cru_symbol)
          action(
              permission_level{ _reserve, "active"_n },
              _token_contract, "transfer"_n,
              std::make_tuple( _reserve, _self, amount_in_asset, std::string("")) 
          ).send();
        

        if (amount_in_asset.symbol == _wcru_symbol)
          action(
              permission_level{ tokenlock::_interchange, "active"_n },
              _token_contract, "transfer"_n,
              std::make_tuple( _interchange, _self, amount_in_asset, std::string("")) 
          ).send();
          

      }      


    } else { //с parent_id
        auto parent_lock_object = locks.find(parent_id);
        
        eosio::check(algorithm != 3, "WCRU should never have a parent_id");

        if (parent_lock_object != locks.end()){
          uint64_t to_retire_amount = uint64_t(0 - amount_in_asset.amount);
          
          eosio::asset to_retire_asset = asset(to_retire_amount, _cru_symbol);
          
          print("to_retire:", to_retire_asset, ";");

          eosio::check(amount < 0, "Only the ability to reduce balance is available.");

          eosio::check(parent_lock_object -> amount >= to_retire_asset, "Not enought parent balance for retire");

          locks.modify(parent_lock_object, _self, [&](auto &l){
            l.amount = parent_lock_object -> amount + amount_in_asset;
          }); 

          modify_balance(username, amount_in_asset, 0);
          modify_balance(username, amount_in_asset, 1);

          action(
              permission_level{ _self, "active"_n },
              _token_contract, "transfer"_n,
              std::make_tuple( _self, _reserve, to_retire_asset, std::string("")) 
          ).send();


        } else {
          //пользователю начисляется бонус по стейкингу, потом он отказывается от стейкинга и с него списывается определенная сумма (штраф). Вот этот штраф имеет родителя - бонус по стейкингу.
          if (amount < 0) {

            modify_debt(username, amount_in_asset);
            modify_balance(username, amount_in_asset, 0);
            
            uint64_t back_to_reserve_amount = uint64_t(0 - amount_in_asset.amount);
            eosio::asset back_to_reserve_asset = asset(back_to_reserve_amount, _cru_symbol);
            
            action(
              permission_level{ _self, "active"_n },
              _token_contract, "transfer"_n,
              std::make_tuple( _self, _reserve, back_to_reserve_asset, std::string("")) 
            ).send();
                
          } else {
            eosio::check(false, "not possible add tokens to already exist object.");
          } 
      }      
    }


    history.emplace(_self, [&](auto &l){
      l.id = history.available_primary_key();
      l.lock_id = id;
      l.lock_parent_id = parent_id;
      l.username = username;
      l.created = datetime;
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
          } else if (action == "migrate"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::migrate);
          } else if (action == "refresh"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::refresh);
          } else if (action == "withdraw"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::withdraw);
          } else if (action == "updatebal"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::updatebal);
          } else if (action == "setbal"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::setbal);
          } else if (action == "restore"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::restore);
          } else if (action == "intchange"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::intchange);
          } else if (action == "frstake"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::frstake);
          } else if (action == "frunstake"_n.value){
            execute_action(name(receiver), name(code), &tokenlock::frunstake);
          } 
          else if (action == "chlbal"_n.value){
            if (code == tokenlock::_self.value)
              execute_action(name(receiver), name(code), &tokenlock::chlbal);
            
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
