open Msgpack_conv

type uri = string with conv(msgpack) (* Redef of Prelude.uri *)

type freq_item =
    uri * int with conv(msgpack)

type summary = string * string * uri * (freq_item list * freq_item list) * (freq_item list * freq_item list) option
    with conv(msgpack)

type mapping = string * uri * uri
    with conv(msgpack)

let mutind_temp_name : uri -> int -> uri = fun defname typenum ->
    String.concat "-" [defname; "internal"; string_of_int typenum]

let mutind_constr_temp_name : uri -> int -> int -> uri = fun defname typenum constrnum ->
    String.concat "-" [mutind_temp_name defname typenum; string_of_int constrnum]

let mutind_name : uri -> string -> uri = fun defname typename ->
    String.concat "::" [defname; typename]

let mutind_constr_name : uri -> string -> string -> uri = fun defname typename constrname ->
    String.concat "." [mutind_name defname typename; constrname]

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
    fun (corpus, file_path, obj_uri, obj_type_uris_tup, obj_value_uris_tup_opt) ->
        let (obj_type_uris, _) = obj_type_uris_tup in
        Util.print "for";
        Printf.printf "%s (in %s:%s)\n%!" obj_uri corpus file_path;
        Util.print "types";
        print_frequency obj_type_uris;
        match obj_value_uris_tup_opt with
        | Some (obj_value_uris, _) ->
                Util.print "body";
                print_frequency obj_value_uris
        | None -> Util.print "no body"
