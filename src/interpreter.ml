let filter : Prelude.uri list -> Prelude.uri list = 
    List.filter (fun x ->
        not (Str.string_match (Str.regexp ".*\\.var$") x 0)
    )

let rec fetch_uris : (Prelude.uri -> unit) -> Acic.aconstr -> unit = fun yield x ->
    let fetch_uris_prodlike args aconstr =
        let arg_f tup =
            let (_id, _name, aconstr_arg) = tup in
            fetch_uris yield aconstr_arg
        in
        List.iter arg_f args;
        fetch_uris yield aconstr
    in
    match x with
    | Acic.ARel(_, _, _, _) -> ()
    | Acic.AVar(_id, uri) -> yield uri
    | Acic.AEvar(_id, _existential_key, aconstr_list) ->
            List.iter (fetch_uris yield) aconstr_list
    | Acic.ASort(_id, _sorts) -> ()
    | Acic.ACast(_id, aconstr_x, aconstr_y) ->
            fetch_uris yield aconstr_x;
            fetch_uris yield aconstr_y
    | Acic.AProds(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.ALambdas(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.ALetIns(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.AApp(_id, aconstr_list) ->
            List.iter (fetch_uris yield) aconstr_list
    | Acic.AConst(_id, _ens, uri) -> yield uri
    | Acic.AInd(_id, _ens, uri, no_type) ->
            yield (String.concat "-" [uri; string_of_int no_type])
    | Acic.AConstruct(_id, _ens, uri, no_type, no_constr) ->
            yield (String.concat "-" [uri; string_of_int no_type; string_of_int no_constr])
    | Acic.ACase(_id, uri, no_type, pt, it, pts) ->
            yield (String.concat "-" [uri; string_of_int no_type]);
            fetch_uris yield pt;
            fetch_uris yield it;
            List.iter (fetch_uris yield) pts
    | Acic.AFix(_id, no_fun, funlist) ->
            (* no_fun is the i-th element for which the fix is actually defined *)
            let f (_id, _name, _rec_index, fun_type, fun_body) =
                fetch_uris yield fun_type;
                fetch_uris yield fun_body
            in
            f (List.nth funlist no_fun)
    | Acic.ACoFix(_id, no_fun, funlist) ->
            (* no_fun is the i-th element for which the cofix is actually defined *)
            let f (_id, _name, fun_type, fun_body) =
                fetch_uris yield fun_type;
                fetch_uris yield fun_body
            in
            f (List.nth funlist no_fun)

let fetch_type_uris yield x =
    match x with
    | Acic.AConstant(_id, _name, _obj_value_opt, obj_type, _params) -> fetch_uris yield obj_type
    | Acic.AVariable(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) ->
        List.iter (fun (_id, _name, _inductive, arity, constructors) ->
                fetch_uris yield arity;
                List.iter (fun (_id, cons_type) ->
                    fetch_uris yield cons_type
                ) constructors
        ) types

let fetch_body_uris yield x =
    match x with
    | Acic.AConstant(_id, _name, obj_value_opt, _obj_type, _params) -> (
            match obj_value_opt with
            | None -> ()
            | Some obj_value -> fetch_uris yield obj_value
    )
    | Acic.AVariable(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) -> ()
