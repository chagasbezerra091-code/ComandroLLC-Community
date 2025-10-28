(* fastboot_ui.ml - Script de Interface e Verificacao de Seguranca Fastboot (OCaml)
 *
 * Este modulo de alto nivel e executado no lado do host (ou em um ambiente de seguranca no dispositivo)
 * para validar comandos Fastboot criticos antes de serem enviados ao fastboot.cc.
 *)

(* --- Tipos de Dados e Estruturas --- *)

(* Define o tipo de dado para um Hash (SHA-256) *)
type firmware_hash = string

(* Define as particoes que podem ser flasheadas *)
type partition = System | Boot | Recovery | Data

(* Define o tipo de um Comando Fastboot Validado *)
type validated_command = 
  | Getvar of string (* Variavel a obter *)
  | Flash of partition * firmware_hash * int (* Particao, Hash, Tamanho Esperado em Bytes *)
  | Download of int (* Tamanho em Bytes *)
  | Reboot
  | Fail of string (* Mensagem de erro de validacao *)


(* --- Constantes de Segurança --- *)
(* Hashes validos e tamanhos de particao conhecidos. 
 * Se o firmware de entrada nao corresponder a estes valores, a operacao e rejeitada. *)
let known_firmwares = [
  ("system", "f64d0c15a8123...", 200000000); (* 200MB *)
  ("boot", "e9b2f7d3c0545...", 50000000);   (* 50MB *)
]


(* --- Funções de Ajuda e Validacao --- *)

(* Tenta converter uma string para o tipo partition. *)
let string_to_partition str =
  match str with
  | "system" -> Some System
  | "boot" -> Some Boot
  | "recovery" -> Some Recovery
  | "data" -> Some Data
  | _ -> None

(* Valida se a combinacao de particao e hash e conhecida e segura. *)
let validate_firmware partition_name input_hash input_size =
  let rec check_known_firmwares list =
    match list with
    | [] -> false
    | (name, hash, size) :: tail ->
      if name = partition_name && hash = input_hash && size = input_size then
        true
      else
        check_known_firmwares tail
  in
  check_known_firmwares known_firmwares


(* --- Função de Parse e Validação Principal --- *)

(* * @brief Tenta parsear e validar um comando de entrada (simulando a entrada do console do host).
 *)
let parse_and_validate command_line =
  let parts = String.split_on_char ' ' command_line in
  match parts with
  | ["getvar"; var] -> Getvar var
  
  | ["download"; size_hex] ->
    (try
      let size = int_of_string ("0x" ^ size_hex) in
      Download size
    with _ -> Fail ("Tamanho de download invalido: " ^ size_hex))

  | ["flash"; partition_name; hash; size_str] ->
    (match string_to_partition partition_name with
     | Some part ->
       (try
         let size = int_of_string size_str in
         if validate_firmware partition_name hash size then
           Flash (part, hash, size)
         else
           Fail ("Falha de seguranca: Hash/Tamanho invalido para particao " ^ partition_name)
       with _ -> Fail ("Tamanho fornecido (" ^ size_str ^ ") nao e um numero."))
     | None ->
       Fail ("Particao de flash desconhecida: " ^ partition_name))

  | ["reboot"] -> Reboot
  
  | _ -> Fail ("Comando Fastboot malformado ou nao suportado.")


(* --- Exemplo de Uso (Main Loop Simulada) --- *)
let () =
  let command1 = "flash system f64d0c15a8123... 200000000" in
  let command2 = "flash system FAKE_HASH 200000000" in
  let command3 = "getvar bootloader-version" in
  
  let process_command cmd =
    let validated = parse_and_validate cmd in
    match validated with
    | Getvar var -> Printf.printf "OK: Obtendo variavel %s\n" var
    | Download size -> Printf.printf "OK: Preparando download de %d bytes\n" size
    | Flash (part, hash, size) -> Printf.printf "OK: Flash seguro de particao %s (hash %s)\n" (match part with System -> "System" | _ -> "Outra") hash
    | Reboot -> Printf.printf "OK: Emitindo comando de reboot\n"
    | Fail msg -> Printf.printf "ERRO DE VALIDADE: %s\n" msg
  in

  Printf.printf "--- Teste de Validacao OCaml ---\n";
  process_command command1;
  process_command command2;
  process_command command3;
  process_command "download:02000000"; (* Tamanho invalido (exigindo 8 caracteres hex) *)
  process_command "download 1A000000"
