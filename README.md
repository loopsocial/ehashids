ehashids
=====

A NIF for 'hashids.c'. The reason for this library is to accelerate hashids encoding and decoding. Elixir and Erlang
implementations might be too slow for some use cases.

Build
-----

    $ rebar3 compile


Example Usage
-----
    %% Params: ehashids:new(Salt, MinHashLen, Alphabet).
    %% New with defaults also exists see source for details.

    1> R = ehashids:new(<<"mysalt">>, 10, <<"abcdefghijklmnop">>).
    #Ref<0.774867635.3955884033.159343>

    2> {ok, Id} = ehashids:encode(R, [12345]).
    {ok,<<"moadpemeag">>}

    3> ehashids:decode_safe(R, <<"moadpemeag">>).
    {ok,[12345]}

    4> {ok, C} = ehashids:compile(R).
    {ok,{<<98,100,101,103,106,107,108,109,110,111,112,0>>,
    <<103,108,112,98,107,110,106,101,100,109,111,0>>,
    <<109,107,100,101,110,106,103,108,111,112,98,0>>,
    11,
    <<0>>,
    0,
    <<99,102,104,105,0>>,
    4,
    <<97,0>>,
    1,10}}

    5> R1 = ehashids:from_compiled(C).
    #Ref<0.774867635.3955884033.159390>

    6> ehashids:decode_safe(R1, <<"moadpemeag">>).
    {ok,[12345]}
