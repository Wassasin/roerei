let filter : Prelude.uri list -> Prelude.uri list = 
    List.filter (fun x ->
        not (Str.string_match (Str.regexp ".*\\.var$") x 0)
    )

let rec fetch_uris : (Prelude.uri -> int -> unit) -> int -> Acic.aconstr -> unit = fun yield depth x ->
    let d = depth+1 in
    let fetch_uris_prodlike args d aconstr =
        let arg_f tup =
            let (_id, _name, aconstr_arg) = tup in
            yield "special:prod" d;
            fetch_uris yield d aconstr_arg
        in
        List.iter arg_f args;
        fetch_uris yield d aconstr
    in
    match x with
    | Acic.ARel(_, _, _, _) -> ()
    | Acic.AVar(_id, uri) -> yield uri d
    | Acic.AEvar(_id, _existential_key, aconstr_list) ->
            List.iter (fetch_uris yield d) aconstr_list
    | Acic.ASort(_id, _sorts) -> ()
    | Acic.ACast(_id, aconstr_x, aconstr_y) ->
            fetch_uris yield d aconstr_x;
            fetch_uris yield d aconstr_y
    | Acic.AProds(args, aconstr) ->
            fetch_uris_prodlike args d aconstr
    | Acic.ALambdas(args, aconstr) ->
            fetch_uris_prodlike args d aconstr
    | Acic.ALetIns(args, aconstr) ->
            fetch_uris_prodlike args d aconstr
    | Acic.AApp(_id, aconstr_list) ->
            List.iter (fun x -> yield "special:app" d; fetch_uris yield d x) aconstr_list
    | Acic.AConst(_id, _ens, uri) -> yield uri d
    | Acic.AInd(_id, _ens, uri, no_type) ->
            yield (Object.mutind_temp_name uri no_type) d
    | Acic.AConstruct(_id, _ens, uri, no_type, no_constr) ->
            yield (Object.mutind_constr_temp_name uri no_type no_constr) d
    | Acic.ACase(_id, uri, no_type, pt, it, pts) ->
            yield (Object.mutind_temp_name uri no_type) d;
            fetch_uris yield d pt;
            fetch_uris yield d it;
            List.iter (fetch_uris yield d) pts
    | Acic.AFix(_id, no_fun, funlist) ->
            (* no_fun is the i-th element for which the fix is actually defined *)
            let f (_id, _name, _rec_index, fun_type, fun_body) =
                fetch_uris yield d fun_type;
                fetch_uris yield d fun_body
            in
            f (List.nth funlist no_fun)
    | Acic.ACoFix(_id, no_fun, funlist) ->
            (* no_fun is the i-th element for which the cofix is actually defined *)
            let f (_id, _name, fun_type, fun_body) =
                fetch_uris yield d fun_type;
                fetch_uris yield d fun_body
            in
            f (List.nth funlist no_fun)

let fetch_type_uris yield x =
    match x with
    | Acic.AConstant(_id, _name, _obj_value_opt, obj_type, _params) -> fetch_uris yield 0 obj_type
    | Acic.AVariable(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) ->
        List.iter (fun (_id, _name, _inductive, arity, constructors) ->
                fetch_uris yield 0 arity;
                List.iter (fun (_id, cons_type) ->
                    fetch_uris yield 0 cons_type
                ) constructors
        ) types

let fetch_body_uris yield x =
    match x with
    | Acic.AConstant(_id, _name, obj_value_opt, _obj_type, _params) -> (
            match obj_value_opt with
            | None -> ()
            | Some obj_value -> fetch_uris yield 0 obj_value
    )
    | Acic.AVariable(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.ACurrentProof(_, _, _, _, _) -> () (* Placeholder *)
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) -> ()

let fetch_constructors yield x =
    match x with
    | Acic.AInductiveDefinition(_id, types, _params, _no_parms) -> (
        List.iteri (fun i (_id, name, _inductive, _arity, constructors) ->
            yield
                (fun b -> Object.mutind_temp_name b i)
                (fun b -> Object.mutind_name b name);
            List.iteri (fun j (cons_name, _cons_type) ->
                (* j is 1-indexed... I know... *)
                yield
                    (fun b -> Object.mutind_constr_temp_name b i (j+1))
                    (fun b -> Object.mutind_constr_name b name cons_name)
            ) constructors
        ) types
    )
    | _ -> ()
