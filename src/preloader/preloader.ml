module StringSet = Set.Make (String) ;;

let analyze_all : ((Prelude.uri -> int -> unit) -> unit) -> (Object.freq_item list * Object.freq_item list) = fun f ->
    let counter_map : Counter.t ref = ref Counter.empty in
    let minlister_map : Minlister.t ref = ref Minlister.empty in
    f (fun x d ->
        counter_map := Counter.touch x !counter_map;
        minlister_map := Minlister.touch x d !minlister_map
    );
    (Counter.to_list !counter_map, Minlister.to_list !minlister_map)

let repo_merge corpus path =
    Printf.sprintf "%s:%s" corpus path

let main () =
    let loaded_repo = ref StringSet.empty in
    let loaded_mapping = ref StringSet.empty in

    if Storage.repository_exists then (
        Util.print "Repository already exists; loading...";
        Storage.repository_load (fun (corpus, path, _, _, _) ->
            loaded_repo := StringSet.add (repo_merge corpus path) !loaded_repo
        );
        Printf.printf "Ignoring %i entries\n%!" (StringSet.cardinal !loaded_repo)
    ) else ();

    if Storage.mapping_exists then (
        Util.print "Mapping already exists; loading...";
        Storage.mapping_load (fun (path, _, _) ->
            loaded_mapping := StringSet.add path !loaded_mapping
        );
        Printf.printf "Ignoring %i entries\n%!" (StringSet.cardinal !loaded_mapping)
    ) else ();

    let os_repo = open_out_gen [Open_creat; Open_append; Open_binary] 0o644 Storage.repository_file in
    let os_mapping = open_out_gen [Open_creat; Open_append; Open_binary] 0o644 Storage.mapping_file in
    let yield_obj : Object.summary -> unit = fun summary ->
        let (corpus, path, _, _, _) = summary in
        loaded_repo := StringSet.add (repo_merge corpus path) !loaded_repo;
        Printf.fprintf os_repo "%s\n%!" (Msgpack.Serialize.serialize_string (Object.msgpack_of_summary summary))
    in
    let yield_mapping : Object.mapping -> unit = fun mapping ->
        let (path, _, _) = mapping in
        loaded_mapping := StringSet.add path !loaded_mapping;
        Printf.fprintf os_mapping "%s\n%!" (Msgpack.Serialize.serialize_string (Object.msgpack_of_mapping mapping))
    in
    Util.list_dir "./repo" (fun corpus ->
    Util.iter_files_rec (Filename.concat "./repo/" corpus) (fun path ->
        let path_loaded_in_repo = StringSet.mem (repo_merge corpus path) !loaded_repo in
        let path_loaded_in_mapping = StringSet.mem path !loaded_mapping in
        if Filename.check_suffix path ".con.xml.gz" then (
            if path_loaded_in_repo then
                Printf.printf "Skipped %s\n%!" path
            else (
                Util.print path;
                let body_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.body.xml.gz" path in
                let types_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.types.xml.gz" path in
                let types_uri = Acicparser.parse_constanttypes types_path in 
                let (type_id, _type_name, type_aconstr) = Acicparser.parse_constanttype path in
                let (type_uris, type_depths) = analyze_all (fun f -> Interpreter.fetch_uris f 0 type_aconstr) in
                if Sys.file_exists body_path then
                    let (body_id, body_uri, body_aconstr) = Acicparser.parse_constantbody body_path in
                    assert (type_id = body_id);
                    let (body_uris, body_depths) = analyze_all (fun f -> Interpreter.fetch_uris f 0 body_aconstr) in
                    yield_obj (corpus, path, types_uri, (type_uris, type_depths), (Some (body_uris, body_depths)))
                else
                    yield_obj (corpus, path, types_uri, (type_uris, type_depths), None)
            )
        ) else if Filename.check_suffix path ".ind.xml.gz" then (
            if path_loaded_in_repo && path_loaded_in_mapping then
                Printf.printf "Skipped %s\n%!" path
            else (
                Util.print path;
                let types_path = Str.global_replace (Str.regexp "\\.ind\\.xml\\.gz") ".ind.types.xml.gz" path in
                let types_uri = Acicparser.parse_constanttypes types_path in 
                let ind = Acicparser.parse_inductivedef path in
                if not path_loaded_in_repo then (
                    let (type_uris, type_depths) = analyze_all (fun f -> Interpreter.fetch_type_uris f ind) in
                    yield_obj (corpus, path, types_uri, (type_uris, type_depths), None)
                );
                if not path_loaded_in_mapping then (
                    Interpreter.fetch_constructors (fun src_f dest_f -> yield_mapping (path, (src_f types_uri), (dest_f types_uri))) ind
                )
            )
        ) else if List.exists (Filename.check_suffix path) [
            ".ind.types.xml.gz";
            ".con.types.xml.gz"; ".con.body.xml.gz";
            ".var.xml.gz"; ".var.types.xml.gz"
        ] then () (* Irrelevant *)
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    ));
    close_out os_mapping;
    close_out os_repo
let () = main ()

