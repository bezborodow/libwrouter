-module(wrouter).
-on_load(load_nif/0).

-export([new/1, resolve/2]).

-type router() :: reference().
-type route() :: {iodata(), term()}.
-type params() :: #{binary() => binary()}.

-export_type([router/0, route/0, params/0]).

%% @doc Build a router from route pattern and context pairs.
-spec new([route()]) -> {ok, router()} | {error, binary()}.
new(Routes) ->
    new_nif(Routes).

%% @doc Resolve a path to its route context and extracted parameters.
-spec resolve(router(), iodata()) -> {ok, term(), params()} | not_found | {error, binary()}.
resolve(Router, Path) ->
    resolve_nif(Router, Path).

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
