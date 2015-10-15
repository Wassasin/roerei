open Dataset

module UriSet = Set.Make (String) ;;
module UriMap = Map.Make (String) ;;

let main () =
    let objects = ref UriSet.empty in
    let dependencies = ref UriSet.empty in

    Storage.repository_load (fun (_, uri, type_uris, body_uris) ->
        List.iter (fun (x, _) ->
            objects := UriSet.add x !objects;
            dependencies := UriSet.add x !dependencies
        ) type_uris;
        Util.may (List.iter (fun (x, _) ->
            objects := UriSet.add x !objects
        )) body_uris
    );

    let theorems = ref (UriSet.diff !objects !dependencies) in (* Heuristic by Kaliszyk *)

    Printf.printf "Dependencies: %i\n%!" (UriSet.cardinal !dependencies);
    Printf.printf "Theorems:     %i\n%!" (UriSet.cardinal !theorems);

    let dataset : Dataset.t =
        let _rows = UriSet.elements !theorems in 
        let _columns = UriSet.elements !dependencies in
        {
            rows = _rows;
            columns = _columns;
            matrix = Dataset.create_matrix (List.length _columns) (List.length _rows) 0;
        } in
    
    let column_map = ref UriMap.empty in
    List.iteri (fun i x -> column_map := UriMap.add x i !column_map) dataset.columns;

    let row_map = ref UriMap.empty in
    List.iteri (fun i x -> row_map := UriMap.add x i !row_map) dataset.rows;

    Util.print "Initializing feature matrix...";

    Storage.repository_load (fun (_, uri, type_uris, _) ->
        try
            let y = UriMap.find uri !row_map in
            List.iter (fun (type_uri, type_uri_count) ->
                let x = UriMap.find type_uri !column_map in
                Dataset.set dataset.matrix x y type_uri_count
            ) type_uris
        with Not_found ->
            ()
    );

    let occurances_total = ref 0 in
    let occurances_count = ref 0 in
    Dataset.iter_nonempty (fun _x _y v ->
        occurances_count := !occurances_count + 1;
        occurances_total := !occurances_total + v
    ) dataset.matrix;
    Printf.printf "Occurances:   %i - %i\n%!" !occurances_count !occurances_total;

    let os = open_out_bin Storage.dataset_file in
    Printf.fprintf os "%s%!" (Msgpack.Serialize.serialize_string (Dataset.msgpack_of_t dataset));
    close_out os


let () = main ()
