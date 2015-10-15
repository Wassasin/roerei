let repository_file = "repo.msgpack"
let dataset_file = "dataset.msgpack"

let repository_exists = Sys.file_exists repository_file

let repository_load : (Object.summary -> unit) -> unit = fun f ->
    if not (Sys.file_exists repository_file) then
        raise (Failure "Repo does not yet exist; please run ./preloader first");
    let is = open_in_bin repository_file in
    try
        let line = ref "" in
        while true do
            line := String.concat "" [!line; input_line is];
            try
                let summary = Object.summary_of_msgpack_exn (Msgpack.Serialize.deserialize_string !line) in
                (try
                    f summary
                with e ->
                    close_in_noerr is;
                    raise e
                );
                line := ""
            with Msgpack_conv.Error(_) ->
                line := String.concat "" [!line; "\n"]
        done
    with End_of_file ->
        close_in_noerr is
