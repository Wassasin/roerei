let print s = Printf.printf "%s\n%!" s

let may : ('a -> unit) -> 'a option -> unit = fun f x_opt ->
    match x_opt with
    | None -> ()
    | Some x -> f x

let list_dir dir f =
    Array.iter (fun str ->
        let path = Filename.concat dir str in
        if Sys.is_directory path
            then f str 
    ) (Sys.readdir dir)

let rec iter_files_rec dir f =
    Array.iter (fun str ->
        let path = Filename.concat dir str in
        if Sys.is_directory path
            then iter_files_rec path f
            else f path
    ) (Sys.readdir dir)
