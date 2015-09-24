let print s = Printf.printf "%s\n%!" s

let count_occurances xs =
    let map : Counter.t ref = ref Counter.empty in
    List.iter (fun x -> map := Counter.touch x !map) xs;
    Counter.to_list !map

let print_occurances xs =
    let xs_sorted : (string * int) list = List.sort (fun (stra, ca) (strb, cb) ->
        let ci = - (compare ca cb) in
        if ci != 0 then ci else compare stra strb
    ) (count_occurances xs) in
    List.iter (fun (str, c) -> Printf.printf "%s: %i\n%!" str c) xs_sorted 

let rec iter_files_rec dir f =
    Array.iter (fun str ->
        let path = Filename.concat dir str in
        if Sys.is_directory path
            then iter_files_rec path f
            else f path
    ) (Sys.readdir dir)

let main () =
    iter_files_rec "/home/wgeraedts/src/Coq" (fun path ->
        if Filename.check_suffix path ".con.xml.gz" then (
            print path;
            let body_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.body.xml.gz" path in
            let types_path = Str.global_replace (Str.regexp "\\.con\\.xml\\.gz") ".con.types.xml.gz" path in
            let (type_id, _type_name, type_aconstr) = Parser.parse_constanttype path in
            print "type";
            print_occurances (Interpreter.fetch_uris type_aconstr);
            if Sys.file_exists body_path then
                let (body_id, body_uri, body_aconstr) = Parser.parse_constantbody body_path in
                assert (type_id = body_id);
                print "body";
                print_occurances (Interpreter.fetch_uris body_aconstr)
            else
                print "no body";
            )
        else if Filename.check_suffix path ".ind.xml.gz" then (
            print path;
            let ind = Parser.parse_inductivedef path in
            print_occurances (Interpreter.fetch_type_uris ind))
        else if List.exists (Filename.check_suffix path) [
            ".ind.types.xml.gz";
            ".con.types.xml.gz"; ".con.body.xml.gz";
            ".var.xml.gz"; ".var.types.xml.gz"
        ] then () (* Irrelevant *)
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    )

let () = main ()

