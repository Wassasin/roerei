(* Prelude types as taken from Coq or Coq XML plugin *)
(* See coq/plugins/xml/acic.ml and relevant kernel files *)

open Msgpack_conv
open Json_conv
open Tiny_json

type existential_key = int with conv(json)  (* from Term *)
type identifier = string with conv(json)  (* from Names *)

type id = string with conv (json)  (* the type of the (annotated) node identifiers *)
type uri = string with conv(msgpack, json)

(* Names *)
type module_ident = identifier with conv(json)
type dir_path = module_ident list with conv(json)
type name = Name of identifier | Anonymous with conv(json)

(* Universe from Univ *)
module UniverseLevel = struct
    type t =
        | Set
        | Level of dir_path * int
    with conv(json)
end

type universe =
    | Atom of UniverseLevel.t
    | Max of UniverseLevel.t list * UniverseLevel.t list
with conv(json)

(* Sorts from Term *)
type contents = Pos | Null with conv(json)

type sorts =
    | Prop of contents  (* proposition types *)
    | Type of universe
with conv(json)

type 'constr context_entry =
    | Decl of 'constr             (* Declaration *)
    | Def  of 'constr * 'constr   (* Definition; the second argument (the type) *)
                                  (* is not present in the DTD, but is needed   *)

type 'constr hypothesis = identifier * 'constr context_entry
type params = (string * uri list) list
