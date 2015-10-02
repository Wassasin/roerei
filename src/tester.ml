let main () =
    let repo = "repo.msgpack" in
    if not (Sys.file_exists repo) then
        raise (Failure "Repo does not yet exist; please run ./preloader first");
    let is = open_in_bin repo in
    try
        let line = ref "" in
        while true do
            line := String.concat "" [!line; input_line is];
            try
                let (uri, _, _) = Object.summary_of_msgpack_exn (Msgpack.Serialize.deserialize_string !line) in
                Util.print uri;
                (*Object.print_summary summary;*)
                line := ""
            with e ->
                line := String.concat "" [!line; "\n"]
        done
    with End_of_file ->
        close_in_noerr is

let () = main ()
