let main () =
    let repo = "repo.msgpack" in
    if Sys.file_exists repo then
        raise (Failure "Repo already exists; please remove repo before recalling executable");
    let os = open_out repo in
    let yield_obj : Object.summary -> unit = fun x ->
        Printf.fprintf os "%s\n%!" (Msgpack.Serialize.serialize_string (Object.msgpack_of_summary x))
    in
    Util.iter_files_rec "/home/wgeraedts/src/Coq" (fun path ->
        if Filename.check_suffix path ".con.xml.gz" then (
            Util.print path;
            let body_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.body.xml.gz" path in
            let types_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.types.xml.gz" path in
            let (type_id, _type_name, type_aconstr) = Parser.parse_constanttype path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let type_uris = Object.count_occurances (Interpreter.fetch_uris type_aconstr) in
            if Sys.file_exists body_path then
                let (body_id, body_uri, body_aconstr) = Parser.parse_constantbody body_path in
                assert (type_id = body_id);
                let body_uris = Object.count_occurances (Interpreter.fetch_uris body_aconstr) in
                yield_obj (types_uri, type_uris, (Some body_uris))
            else
                yield_obj (types_uri, type_uris, None)
            )
        else if Filename.check_suffix path ".ind.xml.gz" then (
            Util.print path;
            let types_path = Str.global_replace (Str.regexp "\\.ind\\.xml\\.gz") ".ind.types.xml.gz" path in
            let ind = Parser.parse_inductivedef path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let type_uris = Object.count_occurances (Interpreter.fetch_type_uris ind) in
            yield_obj (types_uri, type_uris, None)
            )
        else if List.exists (Filename.check_suffix path) [
            ".ind.types.xml.gz";
            ".con.types.xml.gz"; ".con.body.xml.gz";
            ".var.xml.gz"; ".var.types.xml.gz"
        ] then () (* Irrelevant *)
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    );
    close_out os

let () = main ()

