let str = Printf.sprintf
let exec = Filename.basename Sys.executable_name
let pr_err s = Printf.eprintf "%s:%s\n" exec s

let forget x = ()

let apply f x ~finally y =
    let result = try f x with exn -> finally y; raise exn in
    finally y;
    result

let with_inf f inf v =
    try
        let ic = if inf <> "" then open_in_bin inf else stdin in
        let close ic = if inf <> "" then close_in ic else () in
        Some (apply (f ic) v ~finally:close ic)
    with
    | Sys_error e -> pr_err (str " %s" e); None
    | Failure e -> pr_err (str "%s:%s" inf e); None

let string_of_node node =
    match node with
    | `El_start ((_, name), _) -> str "start of %s" name
    | `El_end -> "end"
    | `Data _ -> "data"
    | `Dtd None -> str "dtd (empty)"
    | `Dtd (Some s) -> str "dtd %s" s

let error () = invalid_arg "parse error"

let accept s i =
    let xi = Xmlm.input i in
    if xi = s then () else (
        pr_err (str "unexpected %s, expected %s" (string_of_node xi) (string_of_node s));
        error ())

let accept_start n i =
    let xi = Xmlm.input i in
    let r = match xi with
    | `El_start (("", m), tags) ->
            if n = m then Some tags else None
    | _ -> None
    in
    match r with
    | Some x -> x
    | None -> pr_err (str "unexpected %s, expected start of %s" (string_of_node xi) n); error ()

let peek_start i =
    let xi = Xmlm.peek i in
    match xi with
    | `El_start (("", n), _) -> n
    | _ -> pr_err (str "unexpected %s, expected arbitrary start" (string_of_node xi)); error()

let accept_end i =
    accept (`El_end) i

let peek_end i =
    let xi = Xmlm.peek i in
    match xi with
    | `El_end -> true
    | _ -> false

let accept_dtd i =
    let xi = Xmlm.input i in
    match xi with
    | `Dtd _ -> ()
    | _ -> pr_err (str "unexpected %s, expected arbitrary dtd" (string_of_node xi)); error ()

let rec skip_to_end i =
if not (Xmlm.eoi i) then
    match Xmlm.peek i with
    | `El_start ((_, n), _) ->
            ignore (Xmlm.input i);
            skip_to_end i;
            accept_end i;
            skip_to_end i
    | `El_end -> ()
    | `Data _ -> ignore (Xmlm.input i); skip_to_end i
    | `Dtd _ -> ignore (Xmlm.input i); skip_to_end i

let ignore_block i =
    let name = peek_start i in
    forget (accept_start name i);
    skip_to_end i;
    accept_end i

let lookup_tag_opt key tags =
    try
        Some (snd (List.find (fun ((_, xkey), _) -> key = xkey) tags))
    with Not_found -> None

let lookup_tag key tags =
    match lookup_tag_opt key tags with
    | Some x -> x
    | None -> pr_err (str "tag %s not present" key); List.iter (fun x -> pr_err (snd (fst x))) tags; error ()

let binder_to_name x =
    match x with
    | Some x -> Prelude.Name(x)
    | None -> Prelude.Anonymous

let rec i_aconstr i =
    match peek_start i with
    | "REL" ->
            let tags = accept_start "REL" i in
            let id = lookup_tag "id" tags in
            let value = int_of_string (lookup_tag "value" tags) in
            let idref = lookup_tag "idref" tags in
            let binder = lookup_tag "binder" tags in
            accept_end i;
            Acic.ARel(id, value, idref, binder)
    | "PROD" ->
            forget (accept_start "PROD" i);

            let decls = ref [] in
            while peek_start i = "decl" do
                let tags = accept_start "decl" i in
                let id = lookup_tag "id" tags in
                let name = binder_to_name(lookup_tag_opt "binder" tags) in
                decls := (id, name, i_aconstr i) :: !decls;
                accept_end i
            done;
            decls := List.rev !decls;

            forget (accept_start "target" i);
            let target = i_aconstr i in
            accept_end i;

            accept_end i;
            Acic.AProds(!decls, target)
    | "LAMBDA" ->
            forget (accept_start "LAMBDA" i);

            let decls = ref [] in
            while peek_start i = "decl" do
                let tags = accept_start "decl" i in
                let id = lookup_tag "id" tags in
                let name = binder_to_name(lookup_tag_opt "binder" tags) in
                decls := (id, name, i_aconstr i) :: !decls;
                accept_end i
            done;
            decls := List.rev !decls;

            forget (accept_start "target" i);
            let target = i_aconstr i in
            accept_end i;

            accept_end i;
            Acic.ALambdas(!decls, target)
    | "APPLY" ->
            let tags = accept_start "APPLY" i in
            let id = lookup_tag "id" tags in

            let aconstr_list = ref [] in
            while not (peek_end i) do
                aconstr_list := (i_aconstr i) :: !aconstr_list
            done;
            aconstr_list := List.rev !aconstr_list;
            
            accept_end i;
            Acic.AApp(id, !aconstr_list)
    | "CONST" ->
            let tags = accept_start "CONST" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "uri" tags in
            accept_end i;
            Acic.AConst(id, (None, []), uri)
    | "MUTIND" ->
            let tags = accept_start "MUTIND" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "uri" tags in
            let no_type = int_of_string (lookup_tag "noType" tags) in
            accept_end i;
            Acic.AInd(id, (None, []), uri, no_type)
    | "FIX" ->
            let tags = accept_start "FIX" i in
            let id = lookup_tag "id" tags in
            let no_fun = int_of_string (lookup_tag "noFun" tags) in
            
            let ainductivefun_list = ref [] in
            while not (peek_end i) do
                let tags = accept_start "FixFunction" i in
                let id = lookup_tag "id" tags in
                let name = lookup_tag "name" tags in
                let rec_index = int_of_string(lookup_tag "recIndex" tags) in
                
                forget (accept_start "type" i);
                let fun_type = i_aconstr i in
                accept_end i;

                forget (accept_start "body" i);
                let fun_body = i_aconstr i in
                accept_end i;

                accept_end i;

                ainductivefun_list := (id, name, rec_index, fun_type, fun_body) :: !ainductivefun_list;
            done;
            ainductivefun_list := List.rev !ainductivefun_list;

            accept_end i;
            Acic.AFix(id, no_fun, !ainductivefun_list)
    | name ->
            ignore_block i;
            pr_err (str "warning:ignored aconstr %s" name);
            Acic.AVar("warning placeholder", "")  (* Placeholder *)


let parse_constanttype src _args =
    let i = Xmlm.make_input ~strip:true (`Channel src) in

    let i_constanttype i =
        let tags = accept_start "ConstantType" i in
        let id = lookup_tag "id" tags in
        let name = lookup_tag "name" tags in

        let aconstr_type = i_aconstr i in
        accept_end i;

        (id, name, aconstr_type)
    in

    let i_constanttype_file i =
        accept_dtd i;
        let ct = i_constanttype i in
        assert (Xmlm.eoi i);
        ct
    in

    i_constanttype_file i

let parse_constantbody src _args =
    let i = Xmlm.make_input ~strip:true (`Channel src) in

    let i_constantbody i =
        let tags = accept_start "ConstantBody" i in
        let id = lookup_tag "id" tags in
        let name = lookup_tag "for" tags in

        let aconstr_type = i_aconstr i in
        accept_end i;

        (id, name, aconstr_type)
    in

    let i_constantbody_file i =
        accept_dtd i;
        let ct = i_constantbody i in
        assert (Xmlm.eoi i);
        ct
    in

    i_constantbody_file i


let main () =
    let ct = with_inf parse_constanttype "to_nat.con.xml" () in
    let cb = with_inf parse_constantbody "to_nat.con.body.xml" () in 
    pr_err "done"

let () = main ()
