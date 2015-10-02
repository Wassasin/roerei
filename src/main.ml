let print s = Printf.printf "%s\n%!" s

let count_occurances : Prelude.uri list -> Object.freq_item list  = fun xs ->
    let map : Counter.t ref = ref Counter.empty in
    List.iter (fun x -> map := Counter.touch x !map) xs;
    Counter.to_list !map

let print_frequency : Object.freq_item list -> unit = fun xs ->
    let xs_sorted : Object.freq_item list = List.sort (fun (stra, ca) (strb, cb) ->
        let ci = - (compare ca cb) in
        if ci != 0 then ci else compare stra strb
    ) xs in
    List.iter (fun (str, c) -> Printf.printf "%s: %i\n%!" str c) xs_sorted 

let rec iter_files_rec dir f =
    Array.iter (fun str ->
        let path = Filename.concat dir str in
        if Sys.is_directory path
            then iter_files_rec path f
            else f path
    ) (Sys.readdir dir)
    
let yield_obj : Object.summary -> unit =
    fun (obj_uri, obj_type_uris, obj_value_uris_opt) ->
    print "for";
    print obj_uri;
    print "types";
    print_frequency obj_type_uris;
    match obj_value_uris_opt with
    | Some obj_value_uris ->
            print "body";
            print_frequency obj_value_uris
    | None -> print "no body"

let main () =
    iter_files_rec "/home/wgeraedts/src/Coq" (fun path ->
        if Filename.check_suffix path ".con.xml.gz" then (
            print path;
            let body_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.body.xml.gz" path in
            let types_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.types.xml.gz" path in
            let (type_id, _type_name, type_aconstr) = Parser.parse_constanttype path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let type_uris = count_occurances (Interpreter.fetch_uris type_aconstr) in
            if Sys.file_exists body_path then
                let (body_id, body_uri, body_aconstr) = Parser.parse_constantbody body_path in
                assert (type_id = body_id);
                let body_uris = count_occurances (Interpreter.fetch_uris body_aconstr) in
                yield_obj (types_uri, type_uris, (Some body_uris))
            else
                yield_obj (types_uri, type_uris, None)
            )
        else if Filename.check_suffix path ".ind.xml.gz" then (
            print path;
            let types_path = Str.global_replace (Str.regexp "\\.ind\\.xml\\.gz") ".ind.types.xml.gz" path in
            let ind = Parser.parse_inductivedef path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let type_uris = count_occurances (Interpreter.fetch_type_uris ind) in
            yield_obj (types_uri, type_uris, None)
            )
        else if List.exists (Filename.check_suffix path) [
            ".ind.types.xml.gz";
            ".con.types.xml.gz"; ".con.body.xml.gz";
            ".var.xml.gz"; ".var.types.xml.gz"
        ] then () (* Irrelevant *)
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    )

let () = main ()

