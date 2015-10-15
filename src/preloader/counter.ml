module StringMap = Map.Make (String) ;;

type t = int StringMap.t

let empty = StringMap.empty

let to_list map = StringMap.bindings map

let touch : string -> t -> t = fun s map ->
    let count =
        try
            StringMap.find s map
        with
            | Not_found -> 0
    in
    StringMap.add s (count + 1) map
