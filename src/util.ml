let print s = Printf.printf "%s\n%!" s

let rec iter_files_rec dir f =
    Array.iter (fun str ->
        let path = Filename.concat dir str in
        if Sys.is_directory path
            then iter_files_rec path f
            else f path
    ) (Sys.readdir dir)
