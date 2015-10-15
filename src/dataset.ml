open Msgpack
open Msgpack_conv

module IntMap = Map.Make (struct type t = int let compare = compare end) ;;

type sparse_matrix 'a = {
    width : int;
    height : int;
    default_value : 'a;
    data : ('a IntMap.t) array;
}

let create_matrix : int -> int -> 'a -> 'a sparse_matrix = fun w h d -> {
    width = w;
    height = h;
    default_value = d;
    data = Array.make h IntMap.empty
}

let get : 'a sparse_matrix -> int -> int -> 'a = fun m x y ->
    assert (x >= 0 && x < m.width);
    assert (y >= 0 && y < m.height);
    try
        IntMap.find x (m.data.(y))
    with Not_found ->
        m.default_value

let set : 'a sparse_matrix -> int -> int -> 'a -> unit = fun m x y v ->
    assert (x >= 0 && x <= m.width);
    assert (y >= 0 && y <= m.height);
    if v = m.default_value then
        m.data.(y) <- IntMap.remove x m.data.(y)
    else
        m.data.(y) <- IntMap.add x v m.data.(y) (* Overwrites previous value *)

let iter_nonempty : (int -> int -> 'a -> unit) -> 'a sparse_matrix -> unit = fun f m ->
    Array.iteri (fun y row ->
        IntMap.iter (fun x v ->
            f x y v
        ) row
    ) m.data

let msgpack_of_sparse_matrix : ('a -> Msgpack.Serialize.t) -> 'a sparse_matrix -> Msgpack.Serialize.t = fun msgpack_of_a m ->
    let rev_elements : ((int * int * 'a) list) ref = ref [] in
    Array.iteri (fun y xs ->
        IntMap.iter (fun x v ->
            rev_elements := (x, y, v) :: !rev_elements
        ) xs
    ) m.data;
    let msgpack_elements = List.rev_map (fun (x, y, v) ->
        `FixArray [
            msgpack_of_int x;
            msgpack_of_int y;
            msgpack_of_a v
        ]
    ) !rev_elements in
    `FixArray [
        msgpack_of_int m.width;
        msgpack_of_int m.height;
        msgpack_of_a m.default_value;
        `Array32 msgpack_elements
    ]

let int_of_msgpack_exn = function
    | `PFixnum n | `Int8 n | `Int16 n
    | `NFixnum n | `Uint8 n   | `Uint16 n ->
        n
    | `Int32 n ->
        Int32.to_int n
    | `Int64 n | `Uint32 n ->
        Int64.to_int n
    | `Uint64 n ->
        Big_int.int_of_big_int n
    | _ ->
        failwith "Int expected"

let array_of_msgpack_exn = function
    | `FixArray xs | `Array16 xs | `Array32 xs -> xs
    | _ -> failwith "Array expected"

let map_of_msgpack_exn = function
    | `FixMap xs | `Map16 xs | `Map32 xs -> xs
    | _ -> failwith "Map expected"

let sparse_matrix_of_msgpack_exn : (Msgpack.Serialize.t -> 'a) -> Msgpack.Serialize.t -> 'a sparse_matrix = fun a_of_msgpack_exn m ->
    (function
        | [w; h; d; els] ->
                let result = create_matrix
                    (int_of_msgpack_exn w)
                    (int_of_msgpack_exn h)
                    (a_of_msgpack_exn d)
                in
                List.iter (fun xs ->
                    (function
                    | [x; y; v] ->
                        set result
                            (int_of_msgpack_exn x)
                            (int_of_msgpack_exn y)
                            (a_of_msgpack_exn v)
                    | _ -> failwith "Expected 3-tup-Array")
                    (array_of_msgpack_exn xs)
                ) (array_of_msgpack_exn els);
                result
        | _ -> failwith "Expected 4-tup-Array"
    ) (array_of_msgpack_exn m)

type sparse_int_matrix = int sparse_matrix ;;

let msgpack_of_sparse_int_matrix = msgpack_of_sparse_matrix msgpack_of_int
let sparse_int_matrix_of_msgpack_exn = sparse_matrix_of_msgpack_exn int_of_msgpack_exn

type t = {
    rows : Object.uri list;
    columns : Object.uri list;
    matrix : sparse_int_matrix;
} with conv(msgpack_of)


(*
let t_of_msgpack_exn : Msgpack.Serialize.t -> t = fun x -> function
    | [(rows_key, rows_value
*)
