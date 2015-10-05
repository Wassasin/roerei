open Prelude

let filter : uri list -> uri list = 
    List.filter (fun x ->
        not (Str.string_match (Str.regexp ".*\\.var$") x 0)
    )

let fetch_uris x =
    let rec fetch_uris_sub x =
        let fetch_uris_prodlike args aconstr =
            let arg_f tup =
                let (_id, _name, aconstr_arg) = tup in
                fetch_uris_sub aconstr_arg
            in
            List.flatten (fetch_uris_sub aconstr :: List.map arg_f args)
        in
        match x with
        | Acic.ARel(_, _, _, _) -> []
        | Acic.AVar(_id, uri) -> [uri]
        | Acic.AEvar(_id, _existential_key, aconstr_list) ->
            List.flatten (List.map fetch_uris_sub aconstr_list)
        | Acic.ASort(_id, _sorts) -> []
        | Acic.ACast(_id, aconstr_x, aconstr_y) ->
            List.append (fetch_uris_sub aconstr_x) (fetch_uris_sub aconstr_y)
        | Acic.AProds(args, aconstr) ->
                fetch_uris_prodlike args aconstr
        | Acic.ALambdas(args, aconstr) ->
                fetch_uris_prodlike args aconstr
        | Acic.ALetIns(args, aconstr) ->
                fetch_uris_prodlike args aconstr
        | Acic.AApp(_id, aconstr_list) ->
                List.flatten (List.map fetch_uris_sub aconstr_list)
        | Acic.AConst(_id, _ens, uri) -> [uri]
        | Acic.AInd(_id, _ens, uri, no_type) ->
                [String.concat "-" [uri; string_of_int no_type]]
        | Acic.AConstruct(_id, _ens, uri, no_type, no_constr) ->
                [String.concat "-" [uri; string_of_int no_type; string_of_int no_constr]]
        | Acic.ACase(_id, uri, no_type, pt, it, pts) ->
                let uri = String.concat "-" [uri; string_of_int no_type] in
                uri :: List.flatten [
                    fetch_uris_sub pt;
                    fetch_uris_sub it;
                    List.flatten (List.map fetch_uris_sub pts)
                ]
        | Acic.AFix(_id, no_fun, funlist) ->
                assert (no_fun <= 0);
                let f (_id, _name, _rec_index, fun_type, fun_body) =
                    List.flatten (List.map fetch_uris_sub [fun_type; fun_body])
                in
                List.flatten (List.map f funlist)
        | Acic.ACoFix(_id, no_fun, funlist) ->
                assert (no_fun <= 0);
                let f (_id, _name, fun_type, fun_body) =
                    List.flatten (List.map fetch_uris_sub [fun_type; fun_body])
                in
                List.flatten (List.map f funlist)
        in
     filter (fetch_uris_sub x)

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
