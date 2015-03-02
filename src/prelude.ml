(* Prelude types as taken from Coq or Coq XML plugin *)
(* See coq/plugins/xml/acic.ml and relevant kernel files *)

type existential_key = int  (* from Term *)
type identifier = string  (* from Names *)

type id = string  (* the type of the (annotated) node identifiers *)
type uri = string

(* Names *)
type module_ident = identifier
type dir_path = module_ident list
type name = Name of identifier | Anonymous

(* Universe from Univ *)
module UniverseLevel = struct
    type t =
        | Set
        | Level of dir_path * int
end

type universe =
    | Atom of UniverseLevel.t
    | Max of UniverseLevel.t list * UniverseLevel.t list

(* Sorts from Term *)
type contents = Pos | Null

type sorts =
    | Prop of contents  (* proposition types *)
    | Type of universe

type 'constr context_entry =
    | Decl of 'constr             (* Declaration *)
    | Def  of 'constr * 'constr   (* Definition; the second argument (the type) *)
                                  (* is not present in the DTD, but is needed   *)

type 'constr hypothesis = identifier * 'constr context_entry
type params = (string * uri list) list
