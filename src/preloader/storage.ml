let repository_file = "repo.msgpack"
let dataset_file = "dataset.msgpack"
let mapping_file = "mapping.msgpack"

let read_msgpack_file filename yield_line_f yield_obj_f =
    let is = open_in_bin filename in
    try
        let line = ref "" in
        while true do
            line := String.concat "" [!line; input_line is];
            try
                let summary = yield_line_f !line in
                (try
                    yield_obj_f summary
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

let repository_exists = Sys.file_exists repository_file

let repository_load : (Object.summary -> unit) -> unit = fun f ->
    if not (Sys.file_exists repository_file) then
        raise (Failure "Repo does not yet exist; please run ./preloader first");
    read_msgpack_file repository_file
        (fun line -> Object.summary_of_msgpack_exn (Msgpack.Serialize.deserialize_string line))
        f

let mapping_exists = Sys.file_exists mapping_file

let mapping_load : (Object.mapping -> unit) -> unit = fun f ->
    if not (Sys.file_exists mapping_file) then
        raise (Failure "Mapping does not yet exist; please run ./preloader first");
    read_msgpack_file mapping_file
        (fun line -> Object.mapping_of_msgpack_exn (Msgpack.Serialize.deserialize_string line))
        f
