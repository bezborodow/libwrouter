-module(wrouter_tests).

-include_lib("eunit/include/eunit.hrl").

literal_route_test() ->
    {ok, Router} = wrouter:new([{<<"/hello">>, hello}]),
    ?assertEqual({ok, hello, #{}}, wrouter:resolve(Router, <<"/hello">>)),
    ?assertEqual(not_found, wrouter:resolve(Router, <<"/missing">>)).

param_route_test() ->
    {ok, Router} = wrouter:new([{<<"/users/:id">>, users_show}]),
    ?assertEqual(
        {ok, users_show, #{<<"id">> => <<"42">>}},
        wrouter:resolve(Router, <<"/users/42">>)).

wildcard_route_test() ->
    {ok, Router} = wrouter:new([{<<"/files/*">>, files_show}]),
    ?assertEqual(
        {ok, files_show, #{<<"_">> => <<"a/b/c">>}},
        wrouter:resolve(Router, <<"/files/a/b/c">>)).

route_count_test() ->
    {ok, Router} = wrouter:new([
        {<<"/one">>, one},
        {<<"/two">>, two},
        {<<"/three/:id">>, three}
    ]),
    ?assertEqual(3, wrouter:route_count(Router)).

duplicate_route_error_test() ->
    ?assertMatch(
        {error, _},
        wrouter:new([{<<"/dup">>, one}, {<<"/dup">>, two}])).

function_context_test() ->
    Fun = fun(_Req, Params) -> {handled, Params} end,
    {ok, Router} = wrouter:new([{<<"/callbacks/:id">>, Fun}]),
    {ok, Handler, Params} = wrouter:resolve(Router, <<"/callbacks/abc">>),
    ?assert(is_function(Handler, 2)),
    ?assertEqual({handled, #{<<"id">> => <<"abc">>}}, Handler(request, Params)).
