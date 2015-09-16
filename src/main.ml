let print s = Printf.eprintf "%s\n" s

let count_occurances xs =
    let map : Counter.t ref = ref Counter.empty in
    List.iter (fun x -> map := Counter.touch x !map) xs;
    Counter.to_list !map

let print_occurances xs =
    let xs_sorted : (string * int) list = List.sort (fun (stra, ca) (strb, cb) ->
        let ci = - (compare ca cb) in
        if ci != 0 then ci else compare stra strb
    ) (count_occurances xs) in
    List.iter (fun (str, c) -> Printf.eprintf "%s: %i\n" str c) xs_sorted 

let main () =
    let (type_id, type_name, type_aconstr) = Parser.parse_constanttype "to_nat.con.xml.gz" in
    let (body_id, body_name, body_aconstr) = Parser.parse_constantbody "to_nat.con.body.xml.gz" in

    assert (type_id = body_id); (* body_name is better than type_name *)
    let obj = Acic.AConstant (body_id, body_name, Some body_aconstr, type_aconstr, []) in

    let definitions = Interpreter.fetch_uris type_aconstr in
    let dependencies = Interpreter.fetch_uris body_aconstr in

    print "defs";
    print_occurances definitions; 

    print "deps";
    print_occurances dependencies;

    print "done"

let () = main ()

