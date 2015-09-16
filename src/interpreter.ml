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
    | Acic.AInd(_id, _ens, uri, _no_type) -> [uri]
    | Acic.AConstruct(_id, _ens, uri, _no_type, _no_constr) -> [uri]
    | Acic.ACase(_id, uri, _no_type, pt, it, pts) ->
        uri :: List.flatten [
            fetch_uris pt;
            fetch_uris it;
            List.flatten (List.map fetch_uris pts)
        ]
    | Acic.AFix(_id, _no_fun, funlist) ->
        let f (_id, _name, _rec_index, fun_type, fun_body) =
            List.flatten (List.map fetch_uris [fun_type; fun_body])
        in
        List.flatten (List.map f funlist)
    | Acic.ACoFix(_id, _no_fun, funlist) ->
        let f (_id, _name, fun_type, fun_body) =
            List.flatten (List.map fetch_uris [fun_type; fun_body])
        in
        List.flatten (List.map f funlist)
