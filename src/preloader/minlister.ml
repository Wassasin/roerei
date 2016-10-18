module StringMap = Map.Make (String) ;;

type t = int StringMap.t

let empty = StringMap.empty

let to_list map = StringMap.bindings map

let min : int -> int -> int = fun x y ->
    if x <= y then x else y

let touch : string -> int -> t -> t = fun s x map ->
    let count =
        try
            StringMap.find s map
        with
            | Not_found -> x
    in
    StringMap.add s (min x count) map
