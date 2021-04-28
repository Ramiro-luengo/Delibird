#!/bin/sh
./TP-Delibird_Game-Boy BROKER CAUGHT_POKEMON 1 OK
./TP-Delibird_Game-Boy BROKER CAUGHT_POKEMON 2 FAIL

./TP-Delibird_Game-Boy BROKER NEW_POKEMON Pikachu 2 3 1

./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Onyx 4 5

./TP-Delibird_Game-Boy SUSCRIPTOR NEW_POKEMON 10

./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Charmander 4 5
