let main () =
    Storage.repository_load (fun summary ->
        Object.print_summary summary
    )

let () = main ()
