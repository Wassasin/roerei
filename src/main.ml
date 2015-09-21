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
            let (type_id, type_name, type_aconstr) = Parser.parse_constanttype path in
            print_occurances (Interpreter.fetch_uris type_aconstr))
        else if Filename.check_suffix path ".con.body.xml.gz" then (
            print path;
            let (body_id, body_name, body_aconstr) = Parser.parse_constantbody path in
            print_occurances (Interpreter.fetch_uris body_aconstr))
        else if Filename.check_suffix path ".con.types.xml.gz" then ()
        else
            Printf.eprintf "UNKNOWN %s\n%!" path
    )
    (*let (type_id, type_name, type_aconstr) = Parser.parse_constanttype "to_nat.con.xml.gz" in
    let (body_id, body_name, body_aconstr) = Parser.parse_constantbody "to_nat.con.body.xml.gz" in

    assert (type_id = body_id); (* body_name is better than type_name *)
    let obj = Acic.AConstant (body_id, body_name, Some body_aconstr, type_aconstr, []) in

    let definitions = Interpreter.fetch_uris type_aconstr in
    let dependencies = Interpreter.fetch_uris body_aconstr in

    print "defs";
    print_occurances definitions; 

    print "deps";
    print_occurances dependencies;

    print "done"*)

let () = main ()

