{
  description = "eCloak Core — C++ Qt plugin wrapping e_identity_sdk and e_moderation_sdk via FFI by Evice Labs";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
