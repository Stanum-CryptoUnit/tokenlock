
#include "tokenlock.hpp"


using namespace eosio;


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

void tokenlock::chlbal(eosio::name username, eosio::asset balance){
  //require_auth(tokenlock::_token_contract); //auth self or token
  
  tokenlock::chlbal_act(username, balance, 0);
}

void tokenlock::chlbal_act(eosio::name username, eosio::asset balance, uint64_t type){
    //types: 0 - only liquid, 1 - only total, 2 - only frozen
    
    print("on change total balance");
    tokenlock::tbalance_index balances(tokenlock::_self, tokenlock::_self.value);
    
    auto bal = balances.find(username.value);

    if (bal != balances.end()){

      balances.modify(bal, tokenlock::_self, [&](auto &b){
        if (balance.symbol == _op_symbol){
          if (type == 0){
            b.cru_total += balance;
          } else if (type == 1) {
            b.cru_frozen += balance;
          } 
        } 
      });  

    } else {

      balances.emplace(tokenlock::_self, [&](auto &b){
        b.username = username;
        
        if (balance.symbol == _op_symbol){
          if (type == 0){
            b.cru_total = balance;
            b.cru_frozen = asset(0, _op_symbol);
          } else if (type == 1) {
            b.cru_total = asset(0, _op_symbol);;
            b.cru_frozen = balance;
          } 
        }
      });
    
    }
}

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
  [[eosio::action]] void tokenlock::refresh(eosio::name username, uint64_t id){
    // require_auth(username); //any
    
    users_index users(_self, _self.value);
    auto user = users.find(username.value);
    eosio::check(user != users.end(), "User is not migrated");

    locks_index locks(_self, username.value);
    auto lock = locks.find(id);
    eosio::check(lock != locks.end(), "Lock object is not found");

    auto migrated_at = user -> migrated_at;
    auto created_at = lock-> created;
    eosio::time_point_sec distribution_start_at;
    uint64_t freeze_seconds;

    if (lock->algorithm == 1){
      freeze_seconds = _alg1_freeze_seconds;
    } else if (lock->algorithm == 2) {
      freeze_seconds = _alg2_freeze_seconds;      
    };
    
    distribution_start_at = eosio::time_point_sec(created_at.sec_since_epoch() + freeze_seconds);
    
    bool distribution_is_start = distribution_start_at <= eosio::time_point_sec(now());
    
    print("distribution_is_start:", distribution_is_start, ";");


    uint64_t last_distributed_cycle;
    uint64_t compressed_cycles;

    if (distribution_is_start) {

      if (lock -> last_distribution_at == eosio::time_point_sec(0)){ //Распределяем по контракту впервые
        
        uint64_t cycle_distance = migrated_at.sec_since_epoch() >= created_at.sec_since_epoch() ? (migrated_at.sec_since_epoch() - created_at.sec_since_epoch()) /  _cycle_length : 0;
        uint64_t freeze_cycles = freeze_seconds / _cycle_length;

        last_distributed_cycle = 0;
        compressed_cycles = cycle_distance >= freeze_cycles ? cycle_distance - freeze_cycles : 0;
      
      } else { //Если распределение уже производилось

        last_distributed_cycle = ((lock -> last_distribution_at).sec_since_epoch() - created_at.sec_since_epoch()) / _cycle_length - freeze_seconds / _cycle_length;

      }

      print("last_distributed_cycle1: ", last_distributed_cycle, ";");
      
      uint64_t between_now_and_created_in_seconds = now() - created_at.sec_since_epoch() - freeze_seconds;
      uint64_t current_cycle = between_now_and_created_in_seconds / _cycle_length;
      
      double percent = 0;
      
      print("current_cycle: ", current_cycle, ";");

      eosio::asset to_user_summ = asset(0, _op_symbol);
      eosio::asset asset_amount_to_unfreeze_summ = asset(0, _op_symbol);
      eosio::asset asset_amount_already_unfreezed_summ = asset(0, _op_symbol);

      bool more = true;

      while(last_distributed_cycle <= current_cycle && last_distributed_cycle < 36){


        last_distributed_cycle++;

        double percent = get_current_percent(last_distributed_cycle);

        if (last_distributed_cycle < 36){
          double amount_to_unfreeze = 0;
          double amount_already_unfreezed = 0;
          double last_part = 0;
          eosio::asset asset_last_part = asset(0, _op_symbol);
          
          if (last_distributed_cycle <= compressed_cycles ){
          
            amount_already_unfreezed = percent * (lock -> amount).amount / 100;
            
          } else {
          
            amount_to_unfreeze = percent * (lock -> amount).amount / 100;
          
          }

          eosio::asset asset_amount_to_unfreeze = asset((uint64_t)amount_to_unfreeze, _op_symbol);
          print("asset_amount_to_unfreeze", asset_amount_to_unfreeze);
          eosio::asset asset_amount_already_unfreezed = asset((uint64_t)amount_already_unfreezed, _op_symbol);
          print("asset_amount_already_unfreezed", asset_amount_already_unfreezed);

          if (amount_already_unfreezed > 0){

            eosio::asset to_user_already_unfreezed = asset_amount_already_unfreezed;

            eosio::asset to_reserve = asset(0, _op_symbol);
            eosio::asset debt_in_asset = get_debt(username);
          

            if (debt_in_asset.amount != 0){
          
              if (to_user_already_unfreezed.amount >= debt_in_asset.amount){
                to_reserve = debt_in_asset;
                to_user_already_unfreezed = to_user_already_unfreezed - to_reserve;
              } else {
                to_reserve = to_user_already_unfreezed;
                to_user_already_unfreezed = asset(0, _op_symbol);
              }
              
              modify_debt(username, to_reserve);

            }
            

            if (to_user_already_unfreezed.amount > 0){
              action(
                  permission_level{ _reserve, "active"_n },
                  _token_contract, "transfer"_n,
                  std::make_tuple( _reserve, username, to_user_already_unfreezed, std::string("")) 
              ).send();        
            }
          }

          

          if (last_distributed_cycle == 35){
            asset_last_part = lock -> amount - lock->withdrawed - asset_amount_already_unfreezed - asset_amount_to_unfreeze;
          }

          auto withdrawed = asset_amount_already_unfreezed + asset_amount_to_unfreeze + asset_last_part;

          modify_balance(username, - withdrawed);
          chlbal_act(username, - withdrawed, 0);
          chlbal_act(username, - withdrawed, 1);
          
          
          locks.modify(lock, _self, [&](auto &l){
            l.available += asset_amount_to_unfreeze;
            l.last_distribution_at = eosio::time_point_sec((lock->created).sec_since_epoch() + last_distributed_cycle * _cycle_length + freeze_seconds);
            l.withdrawed = lock->withdrawed + withdrawed;
          });

           
        }
      }
    }
  };

  void tokenlock::modify_balance(eosio::name username, eosio::asset balance_to_add){
    balance_index balances(_self, _self.value);

    auto bal = balances.find(username.value);
    
    if (bal != balances.end()){

      balances.modify(bal, _self, [&](auto &b){
        b.quantity += balance_to_add;
      });  

    } else {
      balances.emplace(_self, [&](auto &b){
        b.username = username;
        b.quantity = balance_to_add;
      });
    }
    
  
  }

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
      debt_in_asset = asset(0, _op_symbol);
    }

    eosio::asset positive_debt_in_asset = asset((uint64_t)(0 - debt_in_asset.amount), debt_in_asset.symbol);

    return positive_debt_in_asset;

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

    eosio::asset debt_in_asset = get_debt(username);

    if ((lock -> available).amount > 0){
      
      eosio::asset to_withdraw = lock -> available;
      
      eosio::asset to_reserve = asset(0, _op_symbol);
      eosio::asset to_user = asset(0, _op_symbol);
      
      eosio::asset positive_debt_in_asset = asset((uint64_t)(0 - debt_in_asset.amount), debt_in_asset.symbol);
      
      if (positive_debt_in_asset.amount != 0){

        if (to_withdraw.amount >= positive_debt_in_asset.amount){
          to_reserve = positive_debt_in_asset;
          to_user = to_withdraw - to_reserve;
        } else {
          to_reserve = to_withdraw;
          to_user = asset(0, _op_symbol);
        }

        modify_debt(username, to_reserve);

      } else {
        to_user = to_withdraw;
      }


      if (to_user.amount > 0)
        action(
            permission_level{ _reserve, "active"_n },
            _token_contract, "transfer"_n,
            std::make_tuple( _reserve, username, to_user, std::string("")) 
        ).send();        


      print("to_user: ", to_user, ";");
      print("debt_still_on_reserve: ", to_reserve, ";");



      if (lock -> withdrawed + lock -> available == lock -> amount){

        locks.erase(lock);
      
      } else {
      
        locks.modify(lock, _self, [&](auto &l){
          l.available = asset(0, _op_symbol);
          l.withdrawed += to_withdraw; 
        });
      
      }


      modify_balance(username, - to_withdraw);
      
     
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
  [[eosio::action]] void tokenlock::migrate(eosio::name username, eosio::public_key public_key){
    require_auth(_self);
    
    users_index users(_self, _self.value);
    auto exist = users.find(username.value);
    eosio::check(exist == users.end(), "User is already migrated");

    users.emplace(_self, [&](auto &u){
      u.username = username;
      u.migrated_at = eosio::time_point_sec(now());
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
    

  };


  [[eosio::action]] void tokenlock::setbal(eosio::name username, eosio::asset balance){
    require_auth(_updater);

    eosio::check( is_account( username ), "user account does not exist");
    eosio::check(balance.symbol == _op_symbol, "wrong symbol");

    balance_index balances(_self, _self.value);
    auto bal = balances.find(username.value);
    
    if (bal != balances.end()){
      balances.erase(bal);

      balances.emplace(_self, [&](auto &b){
        b.username = username;
        b.quantity = balance;
      });

    } else {
      balances.emplace(_self, [&](auto &b){
        b.username = username;
        b.quantity = balance;
      });
    }

    print(balance);
  }

  eosio::asset tokenlock::get_liquid_balance( eosio::name token_contract_account, eosio::name owner, eosio::symbol_code sym_code )
    {
      accounts accountstable( token_contract_account, owner.value );
      auto ac = accountstable.find( sym_code.raw() );
      eosio::asset res = asset(0, _op_symbol);
      if (ac != accountstable.end()){
        res = ac -> balance;
      }
      return res;
    }


  [[eosio::action]] void tokenlock::updatebal(eosio::name username){
    require_auth(_updater);

    history_index history(_self, username.value);
    auto hist_bv = history.begin();

    eosio::asset summ = asset(0, _op_symbol);
    eosio::check( is_account( username ), "user account does not exist");

    while(hist_bv != history.end()) {

      if (hist_bv -> algorithm == 0){
        if (hist_bv -> amount.amount < 0)
          summ += hist_bv -> amount;
      } else {
        summ += hist_bv -> amount;
      }
    
      hist_bv++;
    }        

    
    auto liquid_balance = get_liquid_balance(_token_contract, username, _op_symbol.code());
    print("liquid_balance", liquid_balance, ";");

    balance_index balances(_self, _self.value);
    auto bal = balances.find(username.value);
    
    if (bal != balances.end()){

      balances.erase(bal);

      balances.emplace(_self, [&](auto &b){
        b.username = username;
        b.quantity = summ + liquid_balance;
      });


    } else {
      balances.emplace(_self, [&](auto &b){
        b.username = username;
        b.quantity = summ + liquid_balance;
      });
    }
    
    print(summ);

    balance_index balances2(_self, username.value);
    auto bal2 = balances2.find(username.value);
    if (bal2 != balances2.end())
      balances2.erase(bal2);
  }

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
   *    Инициирует эмиссию токена в контракте eosio.token на аккаунт reserve с целью дальнейшей передачи на аккаунт пользователя username по событию withdraw или немедленно. 
   *    Размороженные объекты немедленно передаются пользователю без записи в таблицу locks. Замороженные объекты требуют совершения действия пользователя для получения размороженных токенов.
   */

  [[eosio::action]] void tokenlock::add(eosio::name username, uint64_t id, uint64_t parent_id, eosio::time_point_sec datetime, uint64_t algorithm, int64_t amount){
    require_auth(_self);
    
    locks_index locks(_self, username.value);
    history_index history(_self, username.value);
    
    auto history_by_id_idx = history.template get_index<"byid"_n>();
    auto exist_hist = history_by_id_idx.find(id);
    eosio::check(exist_hist == history_by_id_idx.end(), "Operation with current ID is already exist");

    print("username:", username, ";");
    print("id:", id, ";");
    print("parent_id:", parent_id, ";");
    print("algorithm:", algorithm, ";");
    print("amount:", amount, ";");

    

    //TODO check for user account exist
    eosio::asset amount_in_asset = asset(amount, _op_symbol);

    modify_balance(username, amount_in_asset);
    
    auto exist_lock = locks.find(id);
    eosio::check(exist_lock == locks.end(), "Lock object with current ID is already exist");

      
    if (parent_id == 0){ //без parent_id
      
      if ( algorithm == 0 ){ //выпуск токенов на пользователя или перевод от пользователя в случае покупки для unlocked CRU
    
          

          debts_index debts(_self, username.value);
          auto debt = debts.find(username.value);
            
          if (amount < 0){

            // Раскоментировать, если после миграции создание долга для пользователя будет запрещено, 
            // а все покупки будут происходить только через контракты на блокчейне.
            // users_index users(_self, _self.value); 
            // auto user = users.find(username.value);
            // eosio::check(user == users.end(), "Cant create debt after migration");


            modify_debt(username, amount_in_asset);
            
            chlbal_act(username, amount_in_asset, 0);
            

            print("debt_added: ", amount_in_asset, ";");
          
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
          l.available = asset(0, _op_symbol);
          l.withdrawed = asset(0, _op_symbol);
        });  
        
        chlbal_act(username, amount_in_asset, 0);
        chlbal_act(username, amount_in_asset, 1);

      }      


    } else { //с parent_id
      
      auto parent_lock_object = locks.find(parent_id);
      
      if (parent_lock_object != locks.end()){
        algorithm = parent_lock_object->algorithm;

        uint64_t to_retire_amount = uint64_t(0 - amount_in_asset.amount);
        
        eosio::asset to_retire = asset(to_retire_amount, _op_symbol);
        
        print("to retire:", to_retire, ";");
        

        eosio::check(amount < 0, "Only the ability to reduce balance is available.");

        eosio::check(parent_lock_object -> amount >= to_retire, "Not enought parent balance for retire");

        locks.modify(parent_lock_object, _self, [&](auto &l){
          l.amount = parent_lock_object -> amount + amount_in_asset;
        }); 

        chlbal_act(username, amount_in_asset, 0);
        chlbal_act(username, amount_in_asset, 1);

      } else {
        //пользователю начисляется бонус по стейкингу, потом он отказывается от стейкинга и с него списывается определенная сумма (штраф). Вот этот штраф имеет родителя - бонус по стейкингу.
        if (amount < 0){

          modify_debt(username, amount_in_asset);
          chlbal_act(username, amount_in_asset, 0);
        
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
          } else if (action == "chlbal"_n.value){
            //if (code == tokenlock::_token_contract.value)
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
