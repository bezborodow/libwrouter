-module(wrouter).
-on_load(load_nif/0).

-export([new/1, resolve/2, route_count/1]).

-type router() :: reference().
-type route() :: {iodata(), term()}.
-type params() :: #{binary() => binary()}.

-export_type([router/0, route/0, params/0]).

-spec new([route()]) -> {ok, router()} | {error, binary()}.
new(Routes) ->
    new_nif(Routes).

-spec resolve(router(), iodata()) -> {ok, term(), params()} | not_found | {error, binary()}.
resolve(Router, Path) ->
    resolve_nif(Router, Path).

-spec route_count(router()) -> non_neg_integer().
route_count(Router) ->
    route_count_nif(Router).

load_nif() ->
    Path =
        case os:getenv("WROUTER_NIF_LIB") of
            false ->
                BeamDir = filename:dirname(code:which(?MODULE)),
                filename:join([BeamDir, "..", "priv", "wrouter_nif"]);
            LibPath ->
                LibPath
        end,
    erlang:load_nif(nif_path(Path), 0).

nif_path(Path) ->
    case filename:extension(Path) of
        ".so" -> filename:rootname(Path);
        _ -> Path
    end.

new_nif(_Routes) ->
    erlang:nif_error(nif_not_loaded).

resolve_nif(_Router, _Path) ->
    erlang:nif_error(nif_not_loaded).

route_count_nif(_Router) ->
    erlang:nif_error(nif_not_loaded).
