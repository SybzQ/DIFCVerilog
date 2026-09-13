% DIFCVerilog Prolog backend
% Copyright (c) 2026 Yubo Shi
% This file is part of DIFCVerilog and is distributed under the GNU GPL.

:- use_module(library(readutil)).
:- use_module(library(lists)).
:- use_module(library(strings)).
:- dynamic node_state/6.
:- initialization(main, main).

main :-
    current_prolog_flag(argv, Args),
    parse_args(Args, Config),
    resolve_files(Config, NodeFile, StepFile, OutFile),
    garbage_collect,
    load_nodes(NodeFile),
    count_nodes(Count),
    format('Loaded ~w nodes.~n', [Count]),
    write('Starting propagation...'), nl,
    setup_call_cleanup(
        open(OutFile, write, OutStream),
        propagate_until_stable(OutStream, StepFile),
        close(OutStream)
    ),
    write('Analysis complete.'), nl.

parse_args([], []).
parse_args(['--stem', Stem | Rest], [stem(Stem) | Config]) :-
    parse_args(Rest, Config).
parse_args(['--nodes', File | Rest], [nodes(File) | Config]) :-
    parse_args(Rest, Config).
parse_args(['--steps', File | Rest], [steps(File) | Config]) :-
    parse_args(Rest, Config).
parse_args(['--out', File | Rest], [out(File) | Config]) :-
    parse_args(Rest, Config).
parse_args(['--help' | _], _) :-
    print_usage,
    halt(0).
parse_args([Bad | _], _) :-
    format(user_error, 'Unknown argument: ~w~n', [Bad]),
    print_usage,
    halt(1).

resolve_files(Config, NodeFile, StepFile, OutFile) :-
    ( member(nodes(NodeFile), Config) -> true
    ; member(stem(Stem), Config) -> atomic_list_concat([Stem, '_nodes.txt'], NodeFile)
    ; NodeFile = 'nodes.txt'
    ),
    ( member(steps(StepFile), Config) -> true
    ; member(stem(Stem), Config) -> atomic_list_concat([Stem, '_steps.txt'], StepFile)
    ; StepFile = 'steps.txt'
    ),
    ( member(out(OutFile), Config) -> true
    ; member(stem(Stem), Config) -> atomic_list_concat([Stem, '_main.out'], OutFile)
    ; OutFile = 'main.out'
    ),
    ensure_readable(NodeFile),
    ensure_readable(StepFile).

ensure_readable(File) :-
    exists_file(File), !.
ensure_readable(File) :-
    format(user_error, 'Input file not found: ~w~n', [File]),
    print_usage,
    halt(1).

print_usage :-
    write(user_error, 'Usage: swipl -q -s DIFCVerilog/difc_backend.pl -- --stem <name>\n'),
    write(user_error, '   or: swipl -q -s DIFCVerilog/difc_backend.pl -- --nodes <nodes.txt> --steps <steps.txt> --out <main.out>\n').

count_nodes(Count) :-
    findall(1, node_state(_,_,_,_,_,_), L),
    length(L, Count).

load_nodes(NodeFile) :-
    retractall(node_state(_,_,_,_,_,_)),
    write('Loading nodes...'), nl,
    setup_call_cleanup(
        open(NodeFile, read, Stream),
        load_nodes_stream(Stream),
        close(Stream)
    ).

load_nodes_stream(Stream) :-
    read_line_to_string(Stream, Line),
    ( Line == end_of_file -> true
    ; ( parse_node_line(Line, NameAtom, BasicMask, PosMask, NegMask) ->
          assertz(node_state(NameAtom, BasicMask, PosMask, NegMask, BasicMask, []))
      ; true
      ),
      load_nodes_stream(Stream)
    ).

parse_node_line(Line, _, _, _, _) :-
    ( Line == "" ; sub_string(Line, 0, 1, _, "#") ), !, fail.
parse_node_line(Line, NameAtom, BasicMask, PosMask, NegMask) :-
    split_string(Line, " ", " \t", RawWords),
    RawWords = [NameStr, BasicStr | Rest],
    atom_string(NameAtom, NameStr),
    str_to_mask(BasicStr, BasicMask),
    ( Rest = [PosStr, NegStr] ->
        str_to_mask(PosStr, PosMask),
        str_to_mask(NegStr, NegMask)
    ; Rest = [PosStr] ->
        str_to_mask(PosStr, PosMask),
        NegMask = 0
    ;
        PosMask = 0, NegMask = 0
    ),
    !.

str_to_mask("", 0) :- !.
str_to_mask(Str, Mask) :-
    split_string(Str, ",", " ", Strs),
    exclude(=(""), Strs, CleanStrs),
    ( CleanStrs == [] -> Mask = 0
    ; maplist(number_string, Nums, CleanStrs),
      nums_to_mask(Nums, Mask)
    ).

nums_to_mask([], 0).
nums_to_mask([N|Rest], Mask) :-
    nums_to_mask(Rest, RestMask),
    Bit is 1 << (N - 1),
    Mask is RestMask \/ Bit.

propagate_until_stable(OutStream, StepFile) :-
    setup_call_cleanup(
        open(StepFile, read, InStream),
        ( set_stream(InStream, buffer_size(1048576)),
          scan_steps_file(InStream, OutStream, false, AnyChanged)
        ),
        close(InStream)
    ),
    garbage_collect,
    ( AnyChanged == true ->
        write(OutStream, 'Changes detected, re-executing steps...\n'),
        propagate_until_stable(OutStream, StepFile)
    ;
        write(OutStream, 'Propagation complete.\n')
    ).

scan_steps_file(InStream, OutStream, ChangedIn, ChangedOut) :-
    read_line_to_string(InStream, Line),
    ( Line == end_of_file ->
        ChangedOut = ChangedIn
    ;
        ( parse_step_line(Line, Step) ->
            ( process_step(OutStream, Step, StepChanged) -> true
            ; StepChanged = false
            ),
            ( StepChanged == true -> NextChanged = true ; NextChanged = ChangedIn ),
            scan_steps_file(InStream, OutStream, NextChanged, ChangedOut)
        ;
            scan_steps_file(InStream, OutStream, ChangedIn, ChangedOut)
        )
    ).

process_step(Stream, step(Desc, From, To, PcP_Nodes), IsChanged) :-
    ( From == 'N_L' ->
        write(Stream, "yes (N_L)\n"),
        IsChanged = false
    ;
        propagate_logic(Stream, From, To, PcP_Nodes, Desc, IsChanged)
    ).

propagate_logic(Stream, FromName, ToName, PcP_Nodes, Desc, IsChanged) :-
    get_node(FromName, _, _, _, Taint1, Path1),
    get_node(ToName, Basic2, Pos2, Neg2, Taint2, _),
    calculate_pc_taint(PcP_Nodes, PcTaintMask),
    Combined is Taint1 \/ PcTaintMask,
    Allow is Basic2 \/ Pos2,
    NewTaint is Taint2 \/ Combined,
    ( NewTaint =:= Taint2 ->
        IsChanged = false
    ;
        IsChanged = true,
        NewPath = [FromName | Path1],
        update_node(ToName, Basic2, Pos2, Neg2, NewTaint, NewPath),
        ( Combined /\ Allow =:= Combined ->
            write(Stream, "yes\n ")
        ;
            write(Stream, "no (violation): "),
            FailPath = [ToName, FromName | Path1],
            print_path(Stream, FailPath),
            format(Stream, "~n[Description]: ~s~n", [Desc])
        )
    ).

get_node(Name, Basic, Pos, Neg, Taint, Path) :-
    ( node_state(Name, Basic, Pos, Neg, Taint, Path) -> true
    ; Basic = 1, Pos = 0, Neg = 0, Taint = 1, Path = []
    ).

update_node(Name, Basic, Pos, Neg, NewTaint, NewPath) :-
    ( retract(node_state(Name, Basic, Pos, Neg, _, _)) ->
        assertz(node_state(Name, Basic, Pos, Neg, NewTaint, NewPath))
    ;
        assertz(node_state(Name, Basic, Pos, Neg, NewTaint, NewPath))
    ).

calculate_pc_taint(PcP_Nodes, Mask) :-
    calculate_pc_taint_acc(PcP_Nodes, 0, Mask).

calculate_pc_taint_acc([], Acc, Acc).
calculate_pc_taint_acc([NodeName|Rest], Acc, Mask) :-
    get_node(NodeName, _, _, _, Taint, _),
    NewAcc is Acc \/ Taint,
    calculate_pc_taint_acc(Rest, NewAcc, Mask).

parse_step_line(Line, _) :-
    ( Line == "" ; sub_string(Line, 0, 1, _, "#") ), !, fail.
parse_step_line(Line, step(Desc, FromAtom, ToAtom, PcP_Atoms)) :-
    sub_string(Line, FirstQ, 1, _, "\""),
    sub_string(Line, LastQ, 1, AfterLast, "\""),
    FirstQ < LastQ,
    DescStart is FirstQ + 1,
    DescLen is LastQ - FirstQ - 1,
    sub_string(Line, DescStart, DescLen, _, Desc),
    sub_string(Line, _, AfterLast, 0, RestStr),
    split_string(RestStr, " ", " \t", RawTokens),
    exclude(=(""), RawTokens, Tokens),
    Tokens = [FromStr, ToStr | RestTokens],
    atom_string(FromAtom, FromStr),
    atom_string(ToAtom, ToStr),
    parse_pc_tokens(RestTokens, PcP_Atoms).

parse_pc_tokens([], []).
parse_pc_tokens([Token|Rest], Result) :-
    ( sub_string(Token, 0, _, _, "pc_p=") ->
        sub_string(Token, 5, _, 0, ValStr),
        ( ValStr == "" ->
            parse_pc_tokens(Rest, Result)
        ;
            split_string(ValStr, ",", "", NameStrs),
            maplist(atom_string, Atoms, NameStrs),
            parse_pc_tokens(Rest, NextResult),
            append(Atoms, NextResult, Result)
        )
    ;
        parse_pc_tokens(Rest, Result)
    ).

print_path(Stream, []) :- write(Stream, "No path found.").
print_path(Stream, [H|T]) :-
    ( T == [] -> write(Stream, "path = ") ; print_path_rec(Stream, T) ),
    ( T == [] -> true ; write(Stream, " -- > ") ),
    write(Stream, H).

print_path_rec(_, []) :- !.
print_path_rec(Stream, [H]) :- !,
    write(Stream, H).
print_path_rec(Stream, [H|T]) :-
    print_path_rec(Stream, T),
    write(Stream, " -- > "),
    write(Stream, H).
