let rec fetch_uris x =
    let fetch_uris_prodlike args aconstr =
        let arg_f tup =
            let (_id, _name, aconstr_arg) = tup in
            fetch_uris aconstr_arg
        in
        List.flatten (fetch_uris aconstr :: List.map arg_f args)
    in
    match x with
    | Acic.ARel(_, _, _, _) -> []
    | Acic.AVar(_id, uri) -> [uri]
    | Acic.AEvar(_id, _existential_key, aconstr_list) ->
        List.flatten (List.map fetch_uris aconstr_list)
    | Acic.ASort(_id, _sorts) -> []
    | Acic.ACast(_id, aconstr_x, aconstr_y) ->
        List.append (fetch_uris aconstr_x) (fetch_uris aconstr_y)
    | Acic.AProds(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.ALambdas(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.ALetIns(args, aconstr) ->
            fetch_uris_prodlike args aconstr
    | Acic.AApp(_id, aconstr_list) ->
        List.flatten (List.map fetch_uris aconstr_list)
    | Acic.AConst(_id, _ens, uri) -> [uri]
    | Acic.AInd(_id, _ens, uri, no_type) ->
            assert (no_type == 0);
            [uri]
    | Acic.AConstruct(_id, _ens, uri, no_type, no_constr) ->
            assert (no_type == 0);
            [String.concat "-" [uri; string_of_int no_constr]]
    | Acic.ACase(_id, uri, no_type, pt, it, pts) ->
            assert (no_type == 0);
        uri :: List.flatten [
            fetch_uris pt;
            fetch_uris it;
            List.flatten (List.map fetch_uris pts)
        ]
    | Acic.AFix(_id, no_fun, funlist) ->
        assert (no_fun == 0);
        let f (_id, _name, _rec_index, fun_type, fun_body) =
            List.flatten (List.map fetch_uris [fun_type; fun_body])
        in
        List.flatten (List.map f funlist)
    | Acic.ACoFix(_id, no_fun, funlist) ->
        assert (no_fun == 0);
        let f (_id, _name, fun_type, fun_body) =
            List.flatten (List.map fetch_uris [fun_type; fun_body])
        in
        List.flatten (List.map f funlist)

let fetch_type_uris x =
    match x with
    | Acic.AConstant(_id, _name, _obj_value_opt, obj_type, _params) -> fetch_uris obj_type
    | Acic.AVariable(_, _, _, _, _) -> [] (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> [] (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) ->
        List.flatten (List.map (fun (_id, _name, _inductive, arity, constructors) ->
            List.flatten (fetch_uris arity :: List.map (fun (_id, cons_type) ->
                fetch_uris cons_type
            ) constructors)
        ) types)

let fetch_body_uris x =
    match x with
    | Acic.AConstant(_id, _name, obj_value_opt, _obj_type, _params) -> (
            match obj_value_opt with
            | None -> []
            | Some obj_value -> fetch_uris obj_value
            )
    | Acic.AVariable(_, _, _, _, _) -> [] (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> [] (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) -> []
