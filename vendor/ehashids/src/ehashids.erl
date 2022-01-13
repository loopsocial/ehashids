-module(ehashids).

-export([
    new/0,
    new/1,
    new/2,
    new/3,
    default_alphabet/0,
    default_salt/0,
    default_separators/0,
    min_hash_length/0,
    min_alphabet_length/0,
    estimate_encoded_size/2,
    encode/2,
    encode_one/2,
    decode/2,
    decode_safe/2,
    compile/1,
    from_compiled/1
]).
-on_load(init/0).

-define(APPNAME, ehashids).
-define(LIBNAME, ehashids).


-opaque hashids_ref() :: binary().
-export_type([hashids_ref/0]).
-opaque compiled_hashids_ref() :: tuple().
-export_type([compiled_hashids_ref/0]).

%% @doc Returns the default hashids alphabet.
%% @end
-spec default_alphabet() -> binary().
default_alphabet() -> <<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890">>.

%% @doc Returns the default hashids salt.
%% @end
-spec default_salt() -> binary().
default_salt() -> <<"">>.

%% @doc Returns the default hashids separators.
%% @end
-spec default_separators() -> binary().
default_separators() -> <<"cfhistuCFHISTU">>.

%% @doc Returns the default hashids minimum alphabet length.
%% @end
-spec min_alphabet_length() -> pos_integer().
min_alphabet_length() -> 16.

%% @doc Returns the default hashids minimum hash length.
%% @end
-spec min_hash_length() -> non_neg_integer().
min_hash_length() -> 0.

%% @doc Creates a new hashids context. With the default parameters.
%% <p>IMPORTANT do not share these references between processes.
%% This can cause stability issues. Avoiding the shuffle overhead
%% at initialization can be accomplished by caching the compiled
%% output of the reference using the `compile/1' function and then
%% recreating the reference in each process.
%% </p>
%% @end
-spec new() -> hashids_ref().
new() ->
    not_loaded(?LINE).

%% @doc Creates a new hashids context. With the default parameters
%% but specify a custom salt.
%% <p>IMPORTANT do not share these references between processes.
%% This can cause stability issues. Avoiding the shuffle overhead
%% at initialization can be accomplished by caching the compiled
%% output of the reference using the `compile/1' function and then
%% recreating the reference in each process.
%% </p>
%% @end
-spec new(Salt :: binary()) -> hashids_ref() | {error, Reason :: atom()}.
new(_Salt) ->
    not_loaded(?LINE).

%% @doc Creates a new hashids context. With the default parameters
%% but specify a custom salt and minimum hash length.
%% <p>IMPORTANT do not share these references between processes.
%% This can cause stability issues. Avoiding the shuffle overhead
%% at initialization can be accomplished by caching the compiled
%% output of the reference using the `compile/1' function and then
%% recreating the reference in each process.
%% </p>
%% @end
-spec new(Salt :: binary(), MinHashLen :: non_neg_integer()) -> hashids_ref() | {error, Reason :: atom()}.
new(_Salt, _MinHashLen) ->
    not_loaded(?LINE).

%% @doc Creates a new hashids context. With the default parameters
%% but specify a custom salt and minimum hash length as well as
%% a custom alphabet.
%% <p>Keep in mind that the alphabet only supports the ASCII
%% character set. If this a problem consider adding UTF-8 support
%% to the underlying library.
%% </p>
%% <p>IMPORTANT do not share these references between processes.
%% This can cause stability issues. Avoiding the shuffle overhead
%% at initialization can be accomplished by caching the compiled
%% output of the reference using the `compile/1' function and then
%% recreating the reference in each process.
%% </p>
%% @end
-spec new(Salt :: binary(), MinHashLen :: non_neg_integer(), Alphabet :: binary()) ->
    hashids_ref() | {error, Reason :: atom()}.
new(_Salt, _MinHashLen, _Alphabet) ->
    not_loaded(?LINE).

%% @doc Estimate the size of the encoding for an input (for REPL use only).
%% @end
-spec estimate_encoded_size(Ref :: hashids_ref(), list(non_neg_integer())) ->
    {ok, Size :: non_neg_integer()} | {error, atom()}.
estimate_encoded_size(_Ref, _Numbers) ->
    not_loaded(?LINE).

%% @doc Encode a list of numbers into a hash id.
%% <p>`Numbers' here are 64 bit unsigned integers internally larger
%% integers will cause an error. For larger values multiple numbers
%% can be used instead.
%% </p>
%% @end
-spec encode(Ref :: hashids_ref(), Numbers :: list(non_neg_integer())) -> {ok, Id :: binary()} | {error, atom()}.
encode(_Ref, _Numbers) ->
    not_loaded(?LINE).

%% @doc Encode a number into a hash id.
%% <p>`Number' here is a 64 bit unsigned integer internally larger
%% values will cause an error. For larger values multiple numbers
%% can be used instead using the `encode/2' function.
%% </p>
%% @end
-spec encode_one(Ref :: hashids_ref(), Number :: non_neg_integer()) -> {ok, Id :: binary()} | {error, atom()}.
encode_one(Ref, Number) -> encode(Ref, [Number]).

%% @doc Decode a hash id into a list of numbers.
%% <p>Note that decode here is not safe the Id can contain additional
%% invalid data. If this is a problem use the equivalent `decode_safe/2'
%% function.
%% </p>
%% @end
-spec decode(Ref :: hashids_ref(), Id :: binary()) -> {ok, Numbers :: list(non_neg_integer())} | {error, atom()}.
decode(_Ref, _Id) ->
    not_loaded(?LINE).

%% @doc Decode a hash id into a list of numbers in a safe way.
%% <p>This function decodes and also checks if the result is
%% commutative with `encode/2'.
%% </p>
%% @end
-spec decode_safe(Ref :: hashids_ref(), Id :: binary()) ->
    {ok, Numbers :: list(non_neg_integer())} | {error, unsafe} | {error, atom()}.
decode_safe(Ref, Id0) ->
    {ok, Numbers} = decode(Ref, Id0),
    {ok, Id1} = encode(Ref, Numbers),
    case Id0 =:= Id1 of
        true -> {ok, Numbers};
        false -> {error, unsafe}
    end.

%% @doc Encode the `hashids_ref()' data in to a tuple to be cached.
%% <p>This data needs to be treated as opaque changing the values
%% can be dangerous!
%% </p>
%% @end
-spec compile(Ref :: hashids_ref()) -> {ok, compiled_hashids_ref()} | {error, atom()}.
compile(_Ref) ->
    not_loaded(?LINE).

%% @doc Create a new `hashids_ref()' from cached data avoiding shuffling.
%% <p>The compiled data needs to be treated as opaque changing the values
%% can be dangerous! Only use an input produced by the `compile/1' function.
%% </p>
%% @end
-spec from_compiled(CompiledData :: compiled_hashids_ref()) -> hashids_ref() | {error, atom()}.
from_compiled(_CompiledData) ->
    not_loaded(?LINE).

%% internal - loads the NIF .so file
init() ->
    SoName = case code:priv_dir(?APPNAME) of
        {error, bad_name} ->
            case filelib:is_dir(filename:join(["..", priv])) of
                true ->
                    filename:join(["..", priv, ?LIBNAME]);
                _ ->
                    filename:join([priv, ?LIBNAME])
            end;
        Dir ->
            filename:join(Dir, ?LIBNAME)
    end,
    erlang:load_nif(SoName, 0).

%% internal - returns an error if the implementation is not loaded
not_loaded(Line) ->
    erlang:nif_error({not_loaded, [{module, ?MODULE}, {line, Line}]}).

