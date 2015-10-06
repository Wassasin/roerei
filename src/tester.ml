module UriSet = Set.Make (String) ;;
module UriMap = Map.Make (String) ;;

let main () =
    let objects = ref UriSet.empty in
    let dependencies = ref UriSet.empty in
    let rows = ref [] in

    Storage.repository_load (fun (_, uri, type_uris, body_uris) ->
        List.iter (fun (x, _) ->
            objects := UriSet.add x !objects;
            dependencies := UriSet.add x !dependencies
        ) type_uris;
        Util.may (List.iter (fun (x, _) ->
            objects := UriSet.add x !objects
        )) body_uris;
        rows := uri :: !rows
    );
    rows := List.rev !rows;

    let theorems = ref (UriSet.diff !objects !dependencies) in (* Heuristic by Kaliszyk *)

    Printf.printf "Rows:         %i\n%!" (List.length !rows);
    Printf.printf "Dependencies: %i\n%!" (UriSet.cardinal !dependencies);
    Printf.printf "Theorems:     %i\n%!" (UriSet.cardinal !theorems);

    let columns = ref (UriSet.elements !dependencies) in
    let matrix = ref (Array.make_matrix (List.length !columns) (List.length !rows) 0) in

    let column_map = ref UriMap.empty in
    List.iteri (fun i x -> column_map := UriMap.add x i !column_map) !columns;

    let row_map = ref UriMap.empty in
    List.iteri (fun i x -> row_map := UriMap.add x i !row_map) !rows;

    Util.print "Initializing feature matrix...";

    Storage.repository_load (fun (_, uri, type_uris, _) ->
        let y = UriMap.find uri !row_map in
        List.iter (fun (type_uri, type_uri_count) ->
            let x = UriMap.find type_uri !column_map in
            (!matrix).(x).(y) <- type_uri_count
        ) type_uris;
    );

    let occurances_count = ref 0 in
    Array.iter (Array.iter (fun b ->
        if b != 0 then
            occurances_count := !occurances_count + 1
        else
            ()
    )) !matrix;
    Printf.printf "Occurances:   %i\n%!" !occurances_count


let () = main ()
