-module(ehashids_tests).

-include_lib("eunit/include/eunit.hrl").

init_test() ->
  R0 = ehashids:new(),
  R1 = ehashids:new(<<"My Salt">>),
  R2 = ehashids:new(<<"My Salt">>),
  ?assert(is_reference(R0)),
  ?assertNotEqual(ehashids:encode_one(R0, 12345), ehashids:encode_one(R1, 12345)),
  ?assertEqual(ehashids:encode_one(R1, 12345), ehashids:encode_one(R2, 12345)),
  ?assertNotEqual(R1, R2).


alphabet_test() ->
  R = ehashids:new(<<"">>, 10, <<"1234567890abcdef">>),
  ?assertEqual({ok, <<"a635945430">>}, ehashids:encode_one(R, 12345)).


min_length_test() ->
  R = ehashids:new(<<"">>, 22),
  ?assertEqual(22, byte_size(element(2, ehashids:encode_one(R, 1)))).


compile_test() ->
  R0 = ehashids:new(<<"">>, 22),
  {ok, C} = ehashids:compile(R0),
  R1 = ehashids:from_compiled(C),
  ?assertEqual(22, byte_size(element(2, ehashids:encode_one(R1, 1)))).

decode_test() ->
  R0 = ehashids:new(),
  R1 = ehashids:new(),
  {ok, Id} = ehashids:encode_one(R0, 123),
  ?assertEqual({ok, [123]}, ehashids:decode(R1, Id)),
  ?assertEqual({ok, [123]}, ehashids:decode_safe(R1, Id)).

estimate_encoded_size_test() ->
  R = ehashids:new(<<"">>, 22),
  Padding = 2,
  ?assertEqual({ok, 22 + Padding}, ehashids:estimate_encoded_size(R, [1888])).
