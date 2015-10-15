module StringSet = Set.Make (String) ;;

let count_all : ((Prelude.uri -> unit) -> unit) -> Object.freq_item list = fun f ->
    let map : Counter.t ref = ref Counter.empty in
    f (fun x -> map := Counter.touch x !map);
    Counter.to_list !map

let main () =
    let loaded = ref StringSet.empty in
    
    if Storage.repository_exists then (
        Util.print "Repository already exists; loading...";
        Storage.repository_load (fun (path, _, _, _) ->
            loaded := StringSet.add path !loaded
        );
        Printf.printf "Ignoring %i entries\n%!" (StringSet.cardinal !loaded)
    ) else ();

    let os = open_out_gen [Open_creat; Open_append; Open_binary] 0o644 Storage.repository_file in
    let yield_obj : Object.summary -> unit = fun summary ->
        let (path, _, _, _) = summary in
        loaded := StringSet.add path !loaded;
        Printf.fprintf os "%s\n%!" (Msgpack.Serialize.serialize_string (Object.msgpack_of_summary summary))
    in
    Util.iter_files_rec "./Coq" (fun path ->
        if StringSet.mem path !loaded then (
            Printf.printf "Skipped %s\n%!" path
        ) else if Filename.check_suffix path ".con.xml.gz" then (
            Util.print path;
            let body_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.body.xml.gz" path in
            let types_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.types.xml.gz" path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let (type_id, _type_name, type_aconstr) = Parser.parse_constanttype path in
            let type_uris = count_all (fun f -> Interpreter.fetch_uris f type_aconstr) in
            if Sys.file_exists body_path then
                let (body_id, body_uri, body_aconstr) = Parser.parse_constantbody body_path in
                assert (type_id = body_id);
                let body_uris = count_all (fun f -> Interpreter.fetch_uris f body_aconstr) in
                yield_obj (path, types_uri, type_uris, (Some body_uris))
            else
                yield_obj (path, types_uri, type_uris, None)
        ) else if Filename.check_suffix path ".ind.xml.gz" then (
            Util.print path;
            let types_path = Str.global_replace (Str.regexp "\\.ind\\.xml\\.gz") ".ind.types.xml.gz" path in
            let types_uri = Parser.parse_constanttypes types_path in 
            let ind = Parser.parse_inductivedef path in
            let type_uris = count_all (fun f -> Interpreter.fetch_type_uris f ind) in
            yield_obj (path, types_uri, type_uris, None)
        ) else if List.exists (Filename.check_suffix path) [
            ".ind.types.xml.gz";
            ".con.types.xml.gz"; ".con.body.xml.gz";
            ".var.xml.gz"; ".var.types.xml.gz"
        ] then () (* Irrelevant *)
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    );
    close_out os
let () = main ()

