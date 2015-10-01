let str = Printf.sprintf
let exec = Filename.basename Sys.executable_name
let pr_err s = Printf.eprintf "%s:%s\n%!" exec s

let forget x = ()

let apply f x ~finally y =
    let result = try f x with exn -> finally y; raise exn in
    finally y;
    result

let protect ~f ~(finally: unit -> unit) =
    let result = ref None in
    try
        result := Some (f ());
        raise Exit
    with
        Exit as e ->
        finally ();
        (match !result with Some x -> x | None -> raise e)
        | e ->
        finally (); raise e

let gzip_open_in file =
    let ch = Gzip.open_in file in
    IO.create_in
    ~read:(fun () -> Gzip.input_char ch)
    ~input:(Gzip.input ch)
    ~close:(fun () -> Gzip.close_in ch)

let std_open_in file =
    let ch = open_in_bin file in
    IO.create_in
    ~read:(fun () -> input_char ch)
    ~input:(input ch)
    ~close:(fun () -> close_in ch)

let xml_exec src f =
    let ch =
        if Filename.check_suffix src ".gz"
            then gzip_open_in src
            else std_open_in src
    in
    protect
        ~f:(fun () ->
            let i = Xmlm.make_input ~strip:true (`Fun (fun () -> IO.read_byte ch)) in
            f i)
        ~finally:(fun () -> IO.close_in ch)

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
    | "VAR" ->
            let tags = accept_start "VAR" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "uri" tags in
            accept_end i;
            Acic.AVar(id, uri)
    | "SORT" ->
            forget (accept_start "SORT" i);
            accept_end i;
            Acic.ARel("", 0, "", "warning SORT placeholder")  (* Placeholder *)
    | "CAST" ->
            let tags = accept_start "CAST" i in
            let id = lookup_tag "id" tags in
            let cast_term = i_aconstr i in
            let cast_type = i_aconstr i in
            accept_end i;
            Acic.ACast(id, cast_term, cast_type)
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
    | "LETIN" ->
            forget (accept_start "LETIN" i);

            let defs = ref [] in
            while peek_start i = "def" do
                let tags = accept_start "def" i in
                let id = lookup_tag "id" tags in
                let name = binder_to_name(lookup_tag_opt "binder" tags) in
                defs := (id, name, i_aconstr i) :: !defs;
                accept_end i;
            done;
            defs := List.rev !defs;

            forget (accept_start "target" i);
            let target = i_aconstr i in
            accept_end i;

            accept_end i;
            Acic.ALetIns(!defs, target)
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
    | "MUTCONSTRUCT" ->
            let tags = accept_start "MUTCONSTRUCT" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "uri" tags in
            let no_type = int_of_string (lookup_tag "noType" tags) in
            let no_constr = int_of_string (lookup_tag "noConstr" tags) in
            accept_end i;
            Acic.AConstruct(id, (None, []), uri, no_type, no_constr)
    | "MUTCASE" ->
            let tags = accept_start "MUTCASE" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "uriType" tags in
            let no_type = int_of_string (lookup_tag "noType" tags) in
            
            forget (accept_start "patternsType" i);
            let patterns_type = i_aconstr i in
            accept_end i;

            forget (accept_start "inductiveTerm" i);
            let inductive_term = i_aconstr i in
            accept_end i;

            let pattern_list = ref [] in
            while not (peek_end i) do
                forget(accept_start "pattern" i);
                let pattern = i_aconstr i in
                accept_end i;
                pattern_list := pattern :: !pattern_list
            done;
            pattern_list := List.rev !pattern_list;

            accept_end i;
            Acic.ACase(id, uri, no_type, patterns_type, inductive_term, !pattern_list)
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
    | "instantiate" ->
            forget (accept_start "instantiate" i);
            let result = i_aconstr i in
            while not (peek_end i) do
                forget (accept_start "arg" i);
                ignore_block i;
                accept_end i;
            done;
            accept_end i;
            result
    | name ->
            ignore_block i;
            pr_err (str "warning:ignored aconstr %s" name);
            Acic.ARel("", 0, "", "warning placeholder")  (* Placeholder *)     

let parse_constanttype src =
    xml_exec src (fun i ->
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
    )

let parse_constantbody src =
    xml_exec src (fun i ->
        let i_constantbody i =
            let tags = accept_start "ConstantBody" i in
            let id = lookup_tag "id" tags in
            let uri = lookup_tag "for" tags in

            let aconstr_body = i_aconstr i in
            accept_end i;

            (id, uri, aconstr_body)
        in

        let i_constantbody_file i =
            accept_dtd i;
            let ct = i_constantbody i in
            assert (Xmlm.eoi i);
            ct
        in

        i_constantbody_file i
    )

let parse_constanttypes src =
    xml_exec src (fun i ->
        let i_constanttypes i =
            let tags = accept_start "InnerTypes" i in
            let types_of = lookup_tag "of" tags in
     
            while not (peek_end i) do
                ignore_block i;
            done;
            
            accept_end i;

            types_of
        in

        let i_constanttypes_file i =
            accept_dtd i;
            let ct = i_constanttypes i in
            assert (Xmlm.eoi i);
            ct
        in

        i_constanttypes_file i
    )

let parse_inductivedef src =
    xml_exec src (fun i ->
        let i_inductivetype i =
            let tags = accept_start "InductiveType" i in
            let id = lookup_tag "id" tags in
            let name = lookup_tag "name" tags in
            let inductive = bool_of_string (lookup_tag "inductive" tags) in
            
            forget (accept_start "arity" i);
            let arity = i_aconstr i in
            accept_end i;

            let constructors = ref [] in
            while not (peek_end i) do
                let tags = accept_start "Constructor" i in
                let name = lookup_tag "name" tags in
                constructors := (name, i_aconstr i) :: !constructors;
                accept_end i;
            done;
            constructors := List.rev !constructors;

            accept_end i;
            (id, name, inductive, arity, !constructors)
        in
        
        let i_inductivedef i =
            let tags = accept_start "InductiveDefinition" i in
            let id = lookup_tag "id" tags in
            let no_params = int_of_string (lookup_tag "noParams" tags) in
            
            let types = ref [] in
            while not (peek_end i) do
                types := i_inductivetype i :: !types;
            done;
            types := List.rev !types;
            assert ((List.length !types) = 1); (* Not clear how this could be different *)
            
            accept_end i;
            Acic.AInductiveDefinition(id, !types, [], no_params)
        in

        let i_inductivedef_file i =
            accept_dtd i;
            let ct = i_inductivedef i in
            assert (Xmlm.eoi i);
            ct
        in

        i_inductivedef_file i
    )
