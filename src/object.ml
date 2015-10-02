open Msgpack_conv

type uri = string with conv(msgpack) (* Redef of Prelude.uri *)

type freq_item =
    uri * int with conv(msgpack)

type summary = uri * freq_item list * freq_item list option
    with conv(msgpack)
