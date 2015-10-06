open Msgpack_conv
open Util

type uri = string with conv(msgpack) (* Redef of Prelude.uri *)

type freq_item =
    uri * int with conv(msgpack)

type summary = string * uri * freq_item list * freq_item list option
    with conv(msgpack)

let count_occurances : Prelude.uri list -> freq_item list  = fun xs ->
    let map : Counter.t ref = ref Counter.empty in
    List.iter (fun x -> map := Counter.touch x !map) xs;
    Counter.to_list !map

let print_frequency : freq_item list -> unit = fun xs ->
    let xs_sorted : freq_item list = List.sort (fun (stra, ca) (strb, cb) ->
        let ci = - (compare ca cb) in
        if ci != 0 then ci else compare stra strb
    ) xs in
    List.iter (fun (str, c) -> Printf.printf "%s: %i\n%!" str c) xs_sorted 

let print_summary : summary -> unit =
    fun (file_path, obj_uri, obj_type_uris, obj_value_uris_opt) ->
        print "for";
        Printf.printf "%s (in %s)\n%!" obj_uri file_path;
        print "types";
        print_frequency obj_type_uris;
        match obj_value_uris_opt with
        | Some obj_value_uris ->
                print "body";
                print_frequency obj_value_uris
        | None -> print "no body"
