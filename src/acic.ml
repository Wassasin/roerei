(* Original file as provided by the HELM Project under the LGPL 2.1 license *)
(* See coq/plugins/xml/acic.ml *)

open Prelude

type aconstr =
  | ARel       of id * int * id * identifier
  | AVar       of id * uri
  | AEvar      of id * existential_key * aconstr list
  | ASort      of id * sorts
  | ACast      of id * aconstr * aconstr
  | AProds     of (id * name * aconstr) list * aconstr
  | ALambdas   of (id * name * aconstr) list * aconstr
  | ALetIns    of (id * name * aconstr) list * aconstr
  | AApp       of id * aconstr list
  | AConst     of id * explicit_named_substitution * uri
  | AInd       of id * explicit_named_substitution * uri * int
  | AConstruct of id * explicit_named_substitution * uri * int * int (* no_type (0-indexed), no_constr (1-indexed) *)
  | ACase      of id * uri * int * aconstr * aconstr * aconstr list
  | AFix       of id * int * ainductivefun list
  | ACoFix     of id * int * acoinductivefun list
and ainductivefun =
 id * identifier * int * aconstr * aconstr
and acoinductivefun =
 id * identifier * aconstr * aconstr
and explicit_named_substitution = id option * (uri * aconstr) list

type acontext = (id * aconstr hypothesis) list
type aconjecture = id * existential_key * acontext * aconstr
type ametasenv = aconjecture list

type aobj =
   AConstant of id * string *                      (* id,           *)
    aconstr option * aconstr *                     (*  value, type, *)
    params                                         (*  parameters   *)
 | AVariable of id *
    string * aconstr option * aconstr *            (* name, body, type *)
    params                                         (*  parameters   *)
 | ACurrentProof of id *
    string * ametasenv *                           (*  name, conjectures, *)
    aconstr * aconstr                              (*  value, type        *)
 | AInductiveDefinition of id *
    anninductiveType list *                        (* inductive types ,      *)
    params * int                                   (*  parameters,n ind. pars*)
and anninductiveType =
 id * identifier * bool * aconstr *           (* typename, inductive, arity *)
  annconstructor list                         (*  constructors              *)
and annconstructor =
 identifier * aconstr                         (* id, type *)
