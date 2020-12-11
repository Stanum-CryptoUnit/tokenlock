#include <eosiolib/multi_index.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.hpp>

// #define IS_DEBUG //commit for production
//#define PERCENT_PRECISION 10000
class [[eosio::contract]] tokenlock : public eosio::contract {

public:
    tokenlock( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds)
    {}
    [[eosio::action]] void add(eosio::name username, uint64_t id, uint64_t parent_id, eosio::time_point_sec datetime, uint64_t algorithm, int64_t amount);
    [[eosio::action]] void migrate(eosio::name username, eosio::public_key public_key, bool self_owner);
    [[eosio::action]] void intchange(eosio::name username, uint64_t lock_id, eosio::asset quantity, uint64_t est_converted_month);
    [[eosio::action]] void refresh(eosio::name username, uint64_t id);
    [[eosio::action]] void withdraw(eosio::name username, uint64_t id);
    [[eosio::action]] void updatebal(eosio::name username);
    [[eosio::action]] void setbal(eosio::name username, eosio::asset frozen_balance);
    [[eosio::action]] void chlbal(eosio::name username, eosio::asset balance, uint64_t type);
    [[eosio::action]] void restore(eosio::name username, eosio::public_key public_key, bool self_owner);
    [[eosio::action]] void frstake(eosio::name username, eosio::asset quantity);
    [[eosio::action]] void frunstake(eosio::name username, eosio::asset quantity);
    
    using chlbal_action = eosio::action_wrapper<"chlbal"_n, &tokenlock::chlbal>;


    struct [[eosio::table]] account {
        eosio::asset    balance;

        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;


    static constexpr eosio::name _self = "tokenlock"_n;   
    static constexpr eosio::name _reserve = "reserve"_n;
    static constexpr eosio::name _updater = "updater"_n;
    static constexpr eosio::name _restore = "restore"_n;
    static constexpr eosio::name _interchange = "interchange"_n;
    static constexpr eosio::name _token_contract = "eosio.token"_n;
    static constexpr eosio::name _system_contract = "eosio"_n;
    static constexpr eosio::name _staker_contract = "staker"_n;
    static constexpr eosio::symbol _cru_symbol     = eosio::symbol(eosio::symbol_code("CRU"), 4);
    static constexpr eosio::symbol _wcru_symbol     = eosio::symbol(eosio::symbol_code("WCRU"), 4);
    static constexpr eosio::symbol _untb_symbol     = eosio::symbol(eosio::symbol_code("UNTB"), 4);
    static constexpr eosio::symbol _usdu_symbol     = eosio::symbol(eosio::symbol_code("USDU"), 4);
    
    
    #ifdef IS_DEBUG
        static constexpr bool _is_debug = true;
        static constexpr uint64_t _alg1_freeze_seconds = 15;
        static constexpr uint64_t _alg2_freeze_seconds = 20;
        static constexpr uint64_t _alg3_freeze_seconds = 30;
        static constexpr uint64_t _cycle_length = 60;

    #else 
        static constexpr bool _is_debug = false;
        static constexpr uint64_t _alg1_freeze_seconds = 25920000;
        static constexpr uint64_t _alg2_freeze_seconds = 47088000;
        static constexpr uint64_t _alg3_freeze_seconds = 31536000;
        static constexpr uint64_t _cycle_length = 2592000;

    #endif

    struct [[eosio::table]] users {
        eosio::name username;
        eosio::time_point_sec migrated_at;
        bool unfreeze_enabled = false;
        eosio::time_point_sec start_unfreeze_at;
    
        uint64_t primary_key() const {return username.value;}
        
        EOSLIB_SERIALIZE(users, (username)(migrated_at)(unfreeze_enabled)(start_unfreeze_at))
    };

    typedef eosio::multi_index<"users"_n, users> users_index;


    struct [[eosio::table]] locks {
        uint64_t id;
        eosio::time_point_sec created;
        eosio::time_point_sec last_distribution_at;
        bool is_staked = false;
        uint64_t algorithm;
        eosio::asset amount;
        eosio::asset available;
        eosio::asset withdrawed;
        eosio::asset staked;
        uint64_t primary_key() const {return id;}
        
        EOSLIB_SERIALIZE(locks, (id)(created)(last_distribution_at)(is_staked)(algorithm)(amount)(available)(withdrawed)(staked))
    };

    typedef eosio::multi_index<"locks"_n, locks> locks_index;

    struct [[eosio::table]] locks3 {
        uint64_t id;
        eosio::asset converted;
        uint64_t est_converted_month;
        
        uint64_t primary_key() const {return id;}
        
        EOSLIB_SERIALIZE(locks3, (id)(converted)(est_converted_month))
    };

    typedef eosio::multi_index<"locks3"_n, locks3> locks_index3;



    struct [[eosio::table]] tbalance {
        eosio::name username;
        eosio::asset cru_total;
        eosio::asset cru_frozen;
        eosio::asset wcru_total;
        eosio::asset wcru_frozen;
        eosio::asset untb_total;
        eosio::asset untb_staked;
        eosio::asset usdu_total;

        uint64_t primary_key() const {return username.value;}
        uint64_t bytotalcru() const {return cru_total.amount;}
        uint64_t byfrozedcru() const {return cru_frozen.amount;}
        uint64_t bytotalwcru() const {return wcru_total.amount;}
        uint64_t byfrozedwcru() const {return wcru_frozen.amount;}
        uint64_t bytotaluntb() const {return untb_total.amount;}
        uint64_t bystakeduntb() const {return untb_staked.amount;}
        uint64_t bytotalusdu() const {return usdu_total.amount;}
        
        EOSLIB_SERIALIZE(tbalance, (username)(cru_total)(cru_frozen)(wcru_total)(wcru_frozen)(untb_total)(untb_staked)(usdu_total))

    };

    typedef eosio::multi_index<"tbalance"_n, tbalance,
        eosio::indexed_by<"bytotalcru"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::bytotalcru>>,
        eosio::indexed_by<"byfrozedcru"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::byfrozedcru>>,
        eosio::indexed_by<"bytotalwcru"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::bytotalwcru>>,
        eosio::indexed_by<"byfrozedwcru"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::byfrozedwcru>>,
        eosio::indexed_by<"bytotaluntb"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::bytotaluntb>>,
        eosio::indexed_by<"bystakeduntb"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::bystakeduntb>>,
        eosio::indexed_by<"bytotalusdu"_n, eosio::const_mem_fun<tbalance, uint64_t, &tbalance::bytotalusdu>>
    > tbalance_index;


    struct [[eosio::table]] tbalance2 {
        eosio::name username;
        eosio::asset cru_staked;
        eosio::asset wcru_staked;
        
        uint64_t primary_key() const {return username.value;}
        uint64_t bystakedcru() const {return cru_staked.amount;}
        uint64_t bystakedwcru() const {return wcru_staked.amount;}
        
        EOSLIB_SERIALIZE(tbalance2, (username)(cru_staked)(wcru_staked))

    };

    typedef eosio::multi_index<"tbalance2"_n, tbalance2,
        eosio::indexed_by<"bystakedcru"_n, eosio::const_mem_fun<tbalance2, uint64_t, &tbalance2::bystakedcru>>,
        eosio::indexed_by<"bystakedwcru"_n, eosio::const_mem_fun<tbalance2, uint64_t, &tbalance2::bystakedwcru>>
    > tbalance2_index;



    struct [[eosio::table]] debts {
        eosio::name username;
        eosio::asset amount;

        uint64_t primary_key() const {return username.value;}
        
        EOSLIB_SERIALIZE(debts, (username)(amount))
    };

    typedef eosio::multi_index<"debts"_n, debts> debts_index;



    struct [[eosio::table]] dhistory{
        uint64_t id;
        eosio::asset amount;
        eosio::time_point_sec timestamp;

        uint64_t primary_key() const {return id;}

        EOSLIB_SERIALIZE(dhistory, (id)(amount)(timestamp))
    };

    typedef eosio::multi_index<"dhistory"_n, dhistory> dhistory_index;


    struct [[eosio::table]] history {
        uint64_t id;
        uint64_t lock_id;
        uint64_t lock_parent_id;
        eosio::name username;

        eosio::time_point_sec created;
        uint64_t algorithm;
        eosio::asset amount;

        uint64_t primary_key() const {return id;}

        uint64_t byusername() const {return username.value;}
        uint64_t byid() const {return lock_id;}
        uint64_t byparentid() const {return lock_parent_id;}
        uint64_t byalgo() const {return algorithm;}

        EOSLIB_SERIALIZE(history, (id)(lock_id)(lock_parent_id)(username)(created)(algorithm)(amount))
    };

    typedef eosio::multi_index<"history"_n, history,
        eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<history, uint64_t, &history::byusername>>,
        eosio::indexed_by<"byid"_n, eosio::const_mem_fun<history, uint64_t, &history::byid>>,
        eosio::indexed_by<"byparentid"_n, eosio::const_mem_fun<history, uint64_t, &history::byparentid>>,
        eosio::indexed_by<"byalgo"_n, eosio::const_mem_fun<history, uint64_t, &history::byalgo>>
    > history_index;







   struct permission_level_weight {
      eosio::permission_level  permission;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      eosio::public_key  key;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct wait_weight {
      uint32_t           wait_sec;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
   };

   struct authority {
      uint32_t                              threshold = 0;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;
      std::vector<wait_weight>              waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
   };



    void apply(uint64_t receiver, uint64_t code, uint64_t action);   
    static eosio::asset get_debt(eosio::name username);   
    static void modify_debt(eosio::name username, eosio::asset amount_to_add);   
    static void refresh_action(eosio::name username, uint64_t id);
    static void modify_balance(eosio::name username, eosio::asset balance, uint64_t type) ;
    static double get_current_percent(uint64_t last_distributed_cycle);
    static void intfrunstake(eosio::name username, eosio::asset quantity);
 
};
